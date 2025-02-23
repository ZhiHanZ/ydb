#include "dq_type_ann.h"

#include <ydb/library/yql/core/yql_expr_type_annotation.h>
#include <ydb/library/yql/core/yql_join.h>
#include <ydb/library/yql/core/yql_opt_utils.h>
#include <ydb/library/yql/core/type_ann/type_ann_core.h>
#include <ydb/library/yql/core/yql_type_helpers.h>
#include <ydb/library/yql/utils/log/log.h>

#include <ydb/library/yql/providers/common/provider/yql_provider.h>

namespace NYql::NDq {

using namespace NYql::NNodes;
using TStatus = NYql::IGraphTransformer::TStatus;

namespace {

const TTypeAnnotationNode* GetDqOutputType(const TDqOutput& output, TExprContext& ctx) {
    auto stageResultTuple = output.Stage().Ref().GetTypeAnn()->Cast<TTupleExprType>();

    ui32 resultIndex;
    if (!TryFromString(output.Index().Value(), resultIndex)) {
        ctx.AddError(TIssue(ctx.GetPosition(output.Pos()),
            TStringBuilder() << "Failed to convert to integer: " << output.Index().Value()));
        return nullptr;
    }

    if (stageResultTuple->GetSize() == 0) {
        ctx.AddError(TIssue(ctx.GetPosition(output.Pos()), "Stage result is empty"));
        return nullptr;
    }

    if (resultIndex >= stageResultTuple->GetSize()) {
        ctx.AddError(TIssue(ctx.GetPosition(output.Pos()),
            TStringBuilder() << "Stage result index out of bounds: " << resultIndex));
        return nullptr;
    }

    auto outputType = stageResultTuple->GetItems()[resultIndex];
    if (!EnsureListType(output.Pos(), *outputType, ctx)) {
        return nullptr;
    }

    return outputType;
}

const TTypeAnnotationNode* GetDqConnectionType(const TDqConnection& node, TExprContext& ctx) {
    return GetDqOutputType(node.Output(), ctx);
}

template <typename TStage>
TStatus AnnotateStage(const TExprNode::TPtr& stage, TExprContext& ctx) {
    if (!EnsureMinMaxArgsCount(*stage, 3, 4, ctx)) {
        return TStatus::Error;
    }

    auto* inputsTuple = stage->Child(TDqStageBase::idx_Inputs);
    auto& programLambda = stage->ChildRef(TDqStageBase::idx_Program);
    auto* settingsTuple = stage->Child(TDqPhyStage::idx_Settings);

    if (!EnsureTuple(*inputsTuple, ctx)) {
        return TStatus::Error;
    }

    if constexpr (std::is_same_v<TStage, TDqPhyStage>) {
        if (!EnsureTuple(*settingsTuple, ctx)) {
            return TStatus::Error;
        }
        for (auto& setting: settingsTuple->Children()) {
            if (!EnsureTupleMinSize(*setting, 1, ctx)) {
                return TStatus::Error;
            }
            if (!EnsureAtom(*setting->Child(0), ctx)) {
                return TStatus::Error;
            }
        }
    }

    if (!EnsureLambda(*programLambda, ctx)) {
        return TStatus::Error;
    }

    if (!EnsureArgsCount(programLambda->Head(), inputsTuple->ChildrenSize(), ctx)) {
        return TStatus::Error;
    }

    TVector<const TTypeAnnotationNode*> argTypes;
    argTypes.reserve(inputsTuple->ChildrenSize());

    for (const auto& input: inputsTuple->Children()) {
        if (!TDqPhyPrecompute::Match(input.Get()) &&
            !(TDqConnection::Match(input.Get()) && !TDqCnValue::Match(input.Get())) &&
            !TDqSource::Match(input.Get()) &&
            !(input->Content() == "KqpTxResultBinding"sv))
        {
            ctx.AddError(TIssue(TStringBuilder() << "Unexpected stage input " << input->Content()));
            return TStatus::Error;
        }

        auto* argType = input->GetTypeAnn();
        if constexpr (std::is_same_v<TStage, TDqPhyStage>) {
            if (TDqConnection::Match(input.Get()) && argType->GetKind() == ETypeAnnotationKind::List) {
                auto* itemType = argType->Cast<TListExprType>()->GetItemType();
                if (!itemType->IsPersistable()) {
                    ctx.AddError(TIssue(ctx.GetPosition(input->Pos()), TStringBuilder()
                                        << "Expected persistable data, but got: "
                                        << *itemType));
                    ctx.AddError(TIssue(ctx.GetPosition(input->Pos()), "Persistable required. Atom, type, key, world, datasink, datasource, callable, resource, stream and lambda are not persistable"));
                    return TStatus::Error;
                }
            }
        }

        if (!TDqPhyPrecompute::Match(input.Get()) && input->Content() != "KqpTxResultBinding") {
            if (argType->GetKind() == ETypeAnnotationKind::List) {
                auto* listItemType = argType->Cast<TListExprType>()->GetItemType();
                if constexpr (std::is_same_v<TStage, TDqPhyStage>) {
                    argType = ctx.MakeType<TStreamExprType>(listItemType);
                } else {
                    argType = ctx.MakeType<TFlowExprType>(listItemType);
                }
            }
        }
        argTypes.emplace_back(argType);
    }

    if (!UpdateLambdaAllArgumentsTypes(programLambda, argTypes, ctx)) {
        return TStatus::Error;
    }

    auto* resultType = programLambda->GetTypeAnn();
    if (!resultType) {
        return TStatus::Repeat;
    }

    TVector<const TTypeAnnotationNode*> programResultTypesTuple;
    if (resultType->GetKind() == ETypeAnnotationKind::Void) {
        // do nothing, return empty tuple as program result
    } else {
        const TTypeAnnotationNode* itemType = nullptr;
        if (resultType->GetKind() == ETypeAnnotationKind::Flow) {
            itemType = resultType->template Cast<TFlowExprType>()->GetItemType();
        } else if (resultType->GetKind() == ETypeAnnotationKind::Stream) {
            itemType = resultType->template Cast<TStreamExprType>()->GetItemType();
        }
        if (itemType) {
            if (itemType->GetKind() == ETypeAnnotationKind::Variant) {
                auto variantType = itemType->Cast<TVariantExprType>()->GetUnderlyingType();
                YQL_ENSURE(variantType->GetKind() == ETypeAnnotationKind::Tuple);
                const auto& items = variantType->Cast<TTupleExprType>()->GetItems();
                programResultTypesTuple.reserve(items.size());
                for (const auto* branchType : items) {
                    programResultTypesTuple.emplace_back(ctx.MakeType<TListExprType>(branchType));
                }
            } else {
                programResultTypesTuple.emplace_back(ctx.MakeType<TListExprType>(itemType));
            }
        } else {
            YQL_ENSURE(resultType->GetKind() != ETypeAnnotationKind::List, "stage: " << stage->Dump());
            programResultTypesTuple.emplace_back(resultType);
        }
    }

    TVector<const TTypeAnnotationNode*> stageResultTypes;
    if (TDqStageBase::idx_Sinks < stage->ChildrenSize()) {
        YQL_ENSURE(stage->Child(TDqStageBase::idx_Sinks)->ChildrenSize() != 0, "Sink list exists but empty, stage: " << stage->Dump());

        auto outputsNumber = programResultTypesTuple.size();
        TVector<TExprNode::TPtr> transformSinks;
        TVector<TExprNode::TPtr> pureSinks;
        transformSinks.reserve(outputsNumber);
        pureSinks.reserve(outputsNumber);
        for (const auto& sink: stage->Child(TDqStageBase::idx_Sinks)->Children()) {
            const ui64 index = FromString(sink->Child(TDqSink::idx_Index)->Content());
            if (index >= outputsNumber) {
                ctx.AddError(TIssue(ctx.GetPosition(stage->Pos()), TStringBuilder()
                    << "Sink/Transform try to process un-existing lambda's output"));
                return TStatus::Error;
            }

            auto transformSettings = sink->Child(TDqSink::idx_Settings)->IsCallable(TTransformSettings::CallableName());
            if (transformSettings) {
                transformSinks.push_back(sink);
            } else {
                pureSinks.push_back(sink);
            }
        }

        if (!transformSinks.empty() && !pureSinks.empty()
            && transformSinks.size() != pureSinks.size()) {

            ctx.AddError(TIssue(ctx.GetPosition(stage->Pos()), TStringBuilder()
                << "Not every transform has a corresponding sink"));
            return TStatus::Error;
        }

        if (!pureSinks.empty()) {
            for (auto sink : pureSinks) {
                sink->SetTypeAnn(resultType);
            }
            stageResultTypes.assign(programResultTypesTuple.begin(), programResultTypesTuple.end());
        } else {
            for (auto sink : transformSinks) {
                auto* sinkType = sink->GetTypeAnn();
                if (sinkType->GetKind() != ETypeAnnotationKind::List
                    && sinkType->GetKind() != ETypeAnnotationKind::Void) {

                    ctx.AddError(TIssue(ctx.GetPosition(sink->Pos()), TStringBuilder()
                        << "Expected List or Void type, but got: " << *sinkType));
                    return TStatus::Error;
                }
                /* auto* itemType = sinkType->Cast<TListExprType>()->GetItemType();
                if (itemType->GetKind() != ETypeAnnotationKind::Struct) {
                    ctx.AddError(TIssue(ctx.GetPosition(sink->Pos()), TStringBuilder()
                        << "Expected List<Struct<...>> type, but got: List<" << *itemType << ">"));
                    return TStatus::Error;
                } */
                stageResultTypes.emplace_back(sinkType);
            }
        }
    } else {
        stageResultTypes.assign(programResultTypesTuple.begin(), programResultTypesTuple.end());
    }

    stage->SetTypeAnn(ctx.MakeType<TTupleExprType>(stageResultTypes));
    return TStatus::Ok;
}

THashMap<TStringBuf, THashMap<TStringBuf, const TTypeAnnotationNode*>>
ParseJoinInputType(const TStructExprType& rowType, TStringBuf tableLabel, TExprContext& ctx, bool optional) {
    THashMap<TStringBuf, THashMap<TStringBuf, const TTypeAnnotationNode*>> result;
    for (auto member : rowType.GetItems()) {
        TStringBuf label, column;
        if (member->GetName().Contains('.')) {
            SplitTableName(member->GetName(), label, column);
        } else {
            column = member->GetName();
        }
        if (label.empty() && tableLabel.empty()) {
            ctx.AddError(TIssue(TStringBuilder() << "Invalid join input type " << FormatType(&rowType)));
            result.clear();
            return result;
        }
        auto memberType = member->GetItemType();
        if (optional && !memberType->IsOptionalOrNull()) {
            memberType = ctx.MakeType<TOptionalExprType>(memberType);
        }
        if (!tableLabel.empty()) {
            result[tableLabel][member->GetName()] = memberType;
        } else {
            result[label][column] = memberType;
        }
    }
    return result;
}

template <bool IsMapJoin>
const TStructExprType* GetDqJoinResultType(TPositionHandle pos, const TStructExprType& leftRowType,
    const TStringBuf& leftLabel, const TStructExprType& rightRowType, const TStringBuf& rightLabel,
    const TStringBuf& joinType, const TDqJoinKeyTupleList& joinKeys, TExprContext& ctx)
{
    // check left
    bool isLeftOptional = IsLeftJoinSideOptional(joinType);
    auto leftType = ParseJoinInputType(leftRowType, leftLabel, ctx, isLeftOptional);
    if (leftType.empty() && joinType != "Cross") {
        TStringStream str; str << "Cannot parse left join input type: ";
        leftRowType.Out(str);
        ctx.AddError(TIssue(ctx.GetPosition(pos), str.Str()));
        return nullptr;
    }

    // check right
    bool isRightOptional = IsRightJoinSideOptional(joinType);
    auto rightType = ParseJoinInputType(rightRowType, rightLabel, ctx, isRightOptional);
    if (rightType.empty() && joinType != "Cross") {
        TStringStream str; str << "Cannot parse right join input type: ";
        rightRowType.Out(str);
        ctx.AddError(TIssue(ctx.GetPosition(pos), str.Str()));
        return nullptr;
    }

    if constexpr (IsMapJoin) {
        if (joinType.StartsWith("Right") || joinType == "Cross") {
            ctx.AddError(TIssue(ctx.GetPosition(pos), TStringBuilder()
                << "Unsupported map join type: " << joinType));
            return nullptr;
        }
    }

    // check join keys
    if (joinKeys.Empty() && joinType != "Cross") {
        ctx.AddError(TIssue(ctx.GetPosition(pos), "No join keys"));
        return nullptr;
    }

    for (const auto& key : joinKeys) {
        auto leftKeyLabel = key.LeftLabel().Value();
        auto leftKeyColumn = key.LeftColumn().Value();
        auto rightKeyLabel = key.RightLabel().Value();
        auto rightKeyColumn = key.RightColumn().Value();

        if (leftLabel && leftLabel != leftKeyLabel) {
            ctx.AddError(TIssue(ctx.GetPosition(pos), "different labels for left table"));
            return nullptr;
        }
        if (rightLabel && rightLabel != rightKeyLabel) {
            ctx.AddError(TIssue(ctx.GetPosition(pos), "different labels for right table"));
            return nullptr;
        }

        auto maybeLeftKeyType = leftType[leftKeyLabel].FindPtr(leftKeyColumn);
        if (!maybeLeftKeyType) {
            ctx.AddError(TIssue(ctx.GetPosition(pos), TStringBuilder()
                << "Left key " << leftKeyLabel << "." << leftKeyColumn << " not found"));
            return nullptr;
        }

        auto maybeRightKeyType = rightType[rightKeyLabel].FindPtr(rightKeyColumn);
        if (!maybeRightKeyType) {
            ctx.AddError(TIssue(ctx.GetPosition(pos), TStringBuilder()
                << "Right key " << rightKeyLabel << "." << rightKeyColumn << " not found"));
            return nullptr;
        }

        auto comparable = CanCompare<true>(*maybeLeftKeyType, *maybeRightKeyType);
        if (comparable != ECompareOptions::Comparable && comparable != ECompareOptions::Optional) {
            ctx.AddError(TIssue(ctx.GetPosition(pos), TStringBuilder()
                << "Not comparable keys: " << leftKeyLabel << "." << leftKeyColumn
                << " and " << rightKeyLabel << "." << rightKeyColumn << ", "
                << FormatType(*maybeLeftKeyType) << " != " << FormatType(*maybeRightKeyType)));
            return nullptr;
        }
    }

    auto addAllMembersFrom = [&ctx](const THashMap<TStringBuf, THashMap<TStringBuf, const TTypeAnnotationNode*>>& type,
        TVector<const TItemExprType*>* result, bool makeOptional = false)
    {
        for (const auto& it : type) {
            for (const auto& it2 : it.second) {
                auto memberName = FullColumnName(it.first, it2.first);
                if (makeOptional && it2.second->GetKind() != ETypeAnnotationKind::Optional) {
                    result->emplace_back(ctx.MakeType<TItemExprType>(memberName, ctx.MakeType<TOptionalExprType>(it2.second)));
                } else {
                    result->emplace_back(ctx.MakeType<TItemExprType>(memberName, it2.second));
                }
            }
        }
    };

    TVector<const TItemExprType*> resultStructItems;
    if (joinType != "RightOnly" && joinType != "RightSemi") {
        addAllMembersFrom(leftType, &resultStructItems, joinType == "Right");
    }
    if (joinType != "LeftOnly" && joinType != "LeftSemi") {
        addAllMembersFrom(rightType, &resultStructItems, joinType == "Left");
    }

    auto rowType = ctx.MakeType<TStructExprType>(resultStructItems);
    return rowType;
}

template <bool IsMapJoin>
const TStructExprType* GetDqJoinResultType(const TExprNode::TPtr& input, bool stream, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 6, ctx)) {
        return nullptr;
    }

    if (!input->Child(TDqJoin::idx_LeftLabel)->IsCallable("Void")) {
        if (!EnsureAtom(*input->Child(TDqJoin::idx_LeftLabel), ctx)) {
            return nullptr;
        }
    }

    if (!input->Child(TDqJoin::idx_RightLabel)->IsCallable("Void")) {
        if (!EnsureAtom(*input->Child(TDqJoin::idx_RightLabel), ctx)) {
            return nullptr;
        }
    }

    if (!EnsureAtom(*input->Child(TDqJoin::idx_JoinType), ctx)) {
        return nullptr;
    }

    if (!EnsureTuple(*input->Child(TDqJoin::idx_JoinKeys), ctx)) {
        return nullptr;
    }

    for (auto& child: input->Child(TDqJoin::idx_JoinKeys)->Children()) {
        if (!EnsureTupleSize(*child, 4, ctx)) {
            return nullptr;
        }
        for (auto& subChild: child->Children()) {
            if (!EnsureAtom(*subChild, ctx)) {
                return nullptr;
            }
        }
    }

    auto join = TDqJoinBase(input);

    auto leftInputType = join.LeftInput().Ref().GetTypeAnn();
    auto rightInputType = join.RightInput().Ref().GetTypeAnn();

    if (stream) {
        if (!EnsureNewSeqType<false, false, true>(join.Pos(), *leftInputType, ctx)) {
            return nullptr;
        }
        if (!EnsureNewSeqType<false, false, true>(join.Pos(), *rightInputType, ctx)) {
            return nullptr;
        }
    } else {
        if (!EnsureNewSeqType<false, true, false>(join.Pos(), *leftInputType, ctx)) {
            return nullptr;
        }
        if (!EnsureNewSeqType<false, true, false>(join.Pos(), *rightInputType, ctx)) {
            return nullptr;
        }
    }

    auto leftInputItemType = GetSeqItemType(leftInputType);
    if (!EnsureStructType(join.Pos(), *leftInputItemType, ctx)) {
        return nullptr;
    }
    auto leftStructType = leftInputItemType->Cast<TStructExprType>();
    auto leftTableLabel = join.LeftLabel().Maybe<TCoAtom>()
        ? join.LeftLabel().Cast<TCoAtom>().Value()
        : TStringBuf("");

    auto rightInputItemType = GetSeqItemType(rightInputType);
    if (!EnsureStructType(join.Pos(), *rightInputItemType, ctx)) {
        return nullptr;
    }
    auto rightStructType = rightInputItemType->Cast<TStructExprType>();
    auto rightTableLabel = join.RightLabel().Maybe<TCoAtom>()
        ? join.RightLabel().Cast<TCoAtom>().Value()
        : TStringBuf("");

    return GetDqJoinResultType<IsMapJoin>(join.Pos(), *leftStructType, leftTableLabel, *rightStructType,
        rightTableLabel, join.JoinType(), join.JoinKeys(), ctx);
}

TStatus AnnotateDqPrecompute(const TExprNode::TPtr& node, TExprContext& ctx) {
    if (!EnsureArgsCount(*node, 1, ctx)) {
        return TStatus::Error;
    }

    node->SetTypeAnn(node->Child(TDqPrecompute::idx_Input)->GetTypeAnn());
    return TStatus::Ok;
}

TStatus AnnotateDqPhyPrecompute(const TExprNode::TPtr& node, TExprContext& ctx) {
    if (!EnsureArgsCount(*node, 1, ctx)) {
        return TStatus::Error;
    }

    auto* cn = node->Child(TDqPhyPrecompute::idx_Connection);
    if (!TDqConnection::Match(cn)) {
        ctx.AddError(TIssue(ctx.GetPosition(cn->Pos()), TStringBuilder() << "Expected DqConnection, got " << cn->Content()));
        return TStatus::Error;
    }

    node->SetTypeAnn(cn->GetTypeAnn());
    return TStatus::Ok;
}

} // unnamed

TStatus AnnotateDqStage(const TExprNode::TPtr& input, TExprContext& ctx) {
    return AnnotateStage<TDqStage>(input, ctx);
}

TStatus AnnotateDqPhyStage(const TExprNode::TPtr& input, TExprContext& ctx) {
    return AnnotateStage<TDqPhyStage>(input, ctx);
}

TStatus AnnotateDqOutput(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 2, ctx)) {
        return TStatus::Error;
    }

    if (!EnsureCallable(*input->Child(TDqOutput::idx_Stage), ctx)) {
        return TStatus::Error;
    }

    if (!TDqStageBase::Match(input->Child(TDqOutput::idx_Stage))) {
        ctx.AddError(TIssue(ctx.GetPosition(input->Child(TDqOutput::idx_Stage)->Pos()), TStringBuilder() << "Expected " << TDqStage::CallableName() << " or " << TDqPhyStage::CallableName()));
        return TStatus::Error;
    }

    if (!EnsureAtom(*input->Child(TDqOutput::idx_Index), ctx)) {
        return TStatus::Error;
    }

    auto resultType = GetDqOutputType(TDqOutput(input), ctx);
    if (!resultType) {
        return TStatus::Error;
    }

    input->SetTypeAnn(resultType);
    return TStatus::Ok;
}

TStatus AnnotateDqConnection(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 1, ctx)) {
        return TStatus::Error;
    }

    if (!EnsureCallable(*input->Child(TDqConnection::idx_Output), ctx)) {
        return TStatus::Error;
    }

    if (!TDqOutput::Match(input->Child(TDqConnection::idx_Output))) {
        ctx.AddError(TIssue(ctx.GetPosition(input->Child(TDqConnection::idx_Output)->Pos()), TStringBuilder() << "Expected " << TDqOutput::CallableName()));
        return TStatus::Error;
    }

    auto resultType = GetDqConnectionType(TDqConnection(input), ctx);
    if (!resultType) {
        return TStatus::Error;
    }

    input->SetTypeAnn(resultType);
    return TStatus::Ok;
}

TStatus AnnotateDqCnMerge(const TExprNode::TPtr& node, TExprContext& ctx) {
    if (!EnsureArgsCount(*node, 2, ctx)) {
        return TStatus::Error;
    }

    if (!EnsureCallable(*node->Child(TDqCnMerge::idx_Output), ctx)) {
        return TStatus::Error;
    }

    if (!TDqOutput::Match(node->Child(TDqCnMerge::idx_Output))) {
        ctx.AddError(TIssue(ctx.GetPosition(node->Child(TDqCnMerge::idx_Output)->Pos()), TStringBuilder() << "Expected " << TDqOutput::CallableName()));
        return TStatus::Error;
    }

    auto cnMerge = TDqCnMerge(node);

    if (!EnsureTupleMinSize(*cnMerge.SortColumns().Ptr(), 1, ctx)) {
        return TStatus::Error;
    }

    auto outputType = GetDqConnectionType(TDqConnection(node), ctx);
    if (!outputType) {
        return TStatus::Error;
    }

    auto itemType = outputType->Cast<TListExprType>()->GetItemType();
    if (!EnsureStructType(node->Pos(), *itemType, ctx)) {
        return TStatus::Error;
    }

    auto structType = itemType->Cast<TStructExprType>();
    for (const auto& column : cnMerge.SortColumns()) {
        if (!EnsureTuple(*column.Ptr(), ctx)) {
            return TStatus::Error;
        }
        if (column.Column().StringValue().empty())
        {
            return TStatus::Error;
        }
        TMaybe<ui32> colIndex = structType->FindItem(column.Column().StringValue());
        if (!colIndex) {
            ctx.AddError(TIssue(ctx.GetPosition(column.Pos()),
                TStringBuilder() << "Missing sort column: " << column.Column().StringValue()));
            return TStatus::Error;
        }
        const TTypeAnnotationNode* colType = (structType->GetItems())[*colIndex]->GetItemType();
        if (colType->GetKind() == ETypeAnnotationKind::Optional) {
            colType = colType->Cast<TOptionalExprType>()->GetItemType();
        }
        if (!EnsureDataType(column.Pos(), *colType, ctx)) {
            ctx.AddError(TIssue(ctx.GetPosition(column.Pos()),
                TStringBuilder() << "For Merge connection column should be Data Expression: " << column.Column().StringValue()));
            return TStatus::Error;
        }
        if (!IsTypeSupportedInMergeCn(colType->Cast<TDataExprType>())) {
            ctx.AddError(TIssue(ctx.GetPosition(column.Pos()),
                TStringBuilder() << "Unsupported type " << colType->Cast<TDataExprType>()->GetName()
                << " for column '" << column.Column().StringValue() << "' in Merge connection."));
            return TStatus::Error;
        }
    }

    node->SetTypeAnn(outputType);
    return TStatus::Ok;
}

TStatus AnnotateDqCnHashShuffle(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 2, ctx)) {
        return TStatus::Error;
    }

    if (!EnsureCallable(*input->Child(TDqCnHashShuffle::idx_Output), ctx)) {
        return TStatus::Error;
    }

    if (!TDqOutput::Match(input->Child(TDqCnHashShuffle::idx_Output))) {
        ctx.AddError(TIssue(ctx.GetPosition(input->Child(TDqCnHashShuffle::idx_Output)->Pos()), TStringBuilder() << "Expected " << TDqOutput::CallableName()));
        return TStatus::Error;
    }

    if (!EnsureTupleMinSize(*input->Child(TDqCnHashShuffle::idx_KeyColumns), 1, ctx)) {
        return TStatus::Error;
    }

    auto outputType = GetDqConnectionType(TDqConnection(input), ctx);
    if (!outputType) {
        return TStatus::Error;
    }

    auto itemType = outputType->Cast<TListExprType>()->GetItemType();
    if (!EnsureStructType(input->Pos(), *itemType, ctx)) {
        return TStatus::Error;
    }

    auto structType = itemType->Cast<TStructExprType>();
    for (const auto& column: input->Child(TDqCnHashShuffle::idx_KeyColumns)->Children()) {
        if (!EnsureAtom(*column, ctx)) {
            return TStatus::Error;
        }
        if (!structType->FindItem(column->Content())) {
            ctx.AddError(TIssue(ctx.GetPosition(column->Pos()),
                TStringBuilder() << "Missing key column: " << column->Content()));
            return TStatus::Error;
        }
    }

    input->SetTypeAnn(outputType);
    return TStatus::Ok;
}

TStatus AnnotateDqCnValue(const TExprNode::TPtr& cnValue, TExprContext& ctx) {
    if (!EnsureArgsCount(*cnValue, 1, ctx)) {
        return TStatus::Error;
    }

    auto& output = cnValue->ChildRef(TDqCnValue::idx_Output);
    if (!TDqOutput::Match(output.Get())) {
        ctx.AddError(TIssue(ctx.GetPosition(output->Pos()), TStringBuilder() << "Expected " << TDqOutput::CallableName()
            << ", got " << output->Content()));
        return TStatus::Error;
    }

    auto* resultType = GetDqOutputType(TDqOutput(output), ctx);
    if (!resultType) {
        return TStatus::Error;
    }

    const TTypeAnnotationNode* outputType = resultType;
    if (resultType->GetKind() == ETypeAnnotationKind::List) {
        outputType = resultType->Cast<TListExprType>()->GetItemType();
    }

    cnValue->SetTypeAnn(outputType);
    return TStatus::Ok;
}

TStatus AnnotateDqCnResult(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 2, ctx)) {
        return TStatus::Error;
    }

    if (!EnsureCallable(*input->Child(TDqCnResult::idx_Output), ctx)) {
        return TStatus::Error;
    }

    if (!TDqOutput::Match(input->Child(TDqCnResult::idx_Output))) {
        ctx.AddError(TIssue(ctx.GetPosition(input->Child(TDqCnResult::idx_Output)->Pos()), TStringBuilder() << "Expected " << TDqOutput::CallableName()));
        return TStatus::Error;
    }

    if (!EnsureTuple(*input->Child(TDqCnResult::idx_ColumnHints), ctx)) {
        return TStatus::Error;
    }

    for (const auto& column: input->Child(TDqCnResult::idx_ColumnHints)->Children()) {
        if (!EnsureAtom(*column, ctx)) {
            return TStatus::Error;
        }
    }

    auto outputType = GetDqConnectionType(TDqConnection(input), ctx);
    if (!outputType) {
        return TStatus::Error;
    }

    input->SetTypeAnn(outputType);
    return TStatus::Ok;
}

TStatus AnnotateDqReplicate(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureMinArgsCount(*input, 3, ctx)) {
        return TStatus::Error;
    }
    auto replicateInput = input->Child(TDqReplicate::idx_Input);
    if (!EnsureFlowType(*replicateInput, ctx)) {
        return TStatus::Error;
    }
    auto inputItemType = replicateInput->GetTypeAnn()->Cast<TFlowExprType>()->GetItemType();
    if (!EnsurePersistableType(replicateInput->Pos(), *inputItemType, ctx)) {
        return TStatus::Error;
    }
    if (!EnsureStructType(replicateInput->Pos(), *inputItemType, ctx)) {
        return TStatus::Error;
    }
    const TTypeAnnotationNode* lambdaInputFlowType = ctx.MakeType<TFlowExprType>(inputItemType);
    TVector<const TTypeAnnotationNode*> outputFlowItems;
    for (ui32 i = 1; i < input->ChildrenSize(); ++i) {
        auto& lambda = input->ChildRef(i);
        if (!EnsureLambda(*lambda, ctx)) {
            return TStatus::Error;
        }
        if (!EnsureArgsCount(lambda->Head(), 1, ctx)) {
            return TStatus::Error;
        }
        if (!UpdateLambdaAllArgumentsTypes(lambda, {lambdaInputFlowType}, ctx)) {
            return TStatus::Error;
        }
        if (!lambda->GetTypeAnn()) {
            return TStatus::Repeat;
        }
        if (!EnsureFlowType(lambda->Pos(), *lambda->GetTypeAnn(), ctx)) {
            return TStatus::Error;
        }
        auto lambdaItemType = lambda->GetTypeAnn()->Cast<TFlowExprType>()->GetItemType();
        if (!EnsurePersistableType(lambda->Pos(), *lambdaItemType, ctx)) {
            return TStatus::Error;
        }
        if (!EnsureStructType(lambda->Pos(), *lambdaItemType, ctx)) {
            return TStatus::Error;
        }
        outputFlowItems.push_back(lambdaItemType);
    }
    auto resultItemType = ctx.MakeType<TVariantExprType>(ctx.MakeType<TTupleExprType>(outputFlowItems));
    input->SetTypeAnn(ctx.MakeType<TFlowExprType>(resultItemType));
    return TStatus::Ok;
}

TStatus AnnotateDqJoin(const TExprNode::TPtr& input, TExprContext& ctx) {
    auto resultRowType = GetDqJoinResultType<false>(input, false, ctx);
    if (!resultRowType) {
        return TStatus::Error;
    }

    input->SetTypeAnn(ctx.MakeType<TListExprType>(resultRowType));
    return TStatus::Ok;
}

TStatus AnnotateDqMapOrDictJoin(const TExprNode::TPtr& input, TExprContext& ctx) {
    auto resultRowType = GetDqJoinResultType<true>(input, true, ctx);
    if (!resultRowType) {
        return TStatus::Error;
    }

    input->SetTypeAnn(ctx.MakeType<TFlowExprType>(resultRowType));
    return TStatus::Ok;
}

TStatus AnnotateDqCrossJoin(const TExprNode::TPtr& input, TExprContext& ctx) {
    auto resultRowType = GetDqJoinResultType<false>(input, true, ctx);
    if (!resultRowType) {
        return TStatus::Error;
    }

    auto join = TDqPhyCrossJoin(input);
    if (join.JoinType().Value() != "Cross") {
        ctx.AddError(TIssue(ctx.GetPosition(join.Pos()), TStringBuilder()
            << "Unexpected join type: " << join.JoinType().Value()));
        return TStatus::Error;
    }

    if (!join.JoinKeys().Empty()) {
        ctx.AddError(TIssue(ctx.GetPosition(join.Pos()), TStringBuilder()
            << "Expected empty join keys for cross join"));
        return TStatus::Error;
    }

    input->SetTypeAnn(ctx.MakeType<TFlowExprType>(resultRowType));
    return TStatus::Ok;
}

TStatus AnnotateDqSource(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 2, ctx)) {
        return TStatus::Error;
    }

    const TExprNode* dataSourceChild = input->Child(TDqSource::idx_DataSource);
    if (!EnsureDataSource(*dataSourceChild, ctx)) {
        return TStatus::Error;
    }

    const TExprNode* settingsChild = input->Child(TDqSource::idx_Settings);
    if (!EnsureCallable(*settingsChild, ctx)) {
        return TStatus::Error;
    }

    input->SetTypeAnn(settingsChild->GetTypeAnn());
    return TStatus::Ok;
}

TStatus AnnotateDqSink(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 3, ctx)) {
        return TStatus::Error;
    }

    const TExprNode* dataSinkChild = input->Child(TDqSink::idx_DataSink);
    if (!EnsureDataSink(*dataSinkChild, ctx)) {
        return TStatus::Error;
    }

    const TExprNode* settingsChild = input->Child(TDqSink::idx_Settings);
    if (!EnsureCallable(*settingsChild, ctx)) {
        return TStatus::Error;
    }

    const TExprNode* indexChild = input->Child(TDqSink::idx_Index);
    if (!EnsureAtom(*indexChild, ctx)) {
        return TStatus::Error;
    }

    input->SetTypeAnn(settingsChild->GetTypeAnn());
    return TStatus::Ok;
}

TStatus AnnotateDqQuery(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 2, ctx)) {
        return TStatus::Error;
    }

    TDqQuery query(input);
    input->SetTypeAnn(query.World().Ref().GetTypeAnn());
    return TStatus::Ok;
}

TStatus AnnotateTransformSettings(const TExprNode::TPtr& input, TExprContext& ctx) {
    if (!EnsureArgsCount(*input, 4U, ctx)) {
        return TStatus::Error;
    }

    const TExprNode* outputArg = input->Child(TTransformSettings::idx_OutputType);
    if (!EnsureTypeWithStructType(*outputArg, ctx)) {
        return IGraphTransformer::TStatus::Error;
    }
    const TTypeAnnotationNode* outputType = outputArg->GetTypeAnn()->Cast<TTypeExprType>()->GetType();

    const TExprNode* inputType = input->Child(TTransformSettings::idx_InputType);
    if (!EnsureTypeWithStructType(*inputType, ctx)) {
        return IGraphTransformer::TStatus::Error;
    }

    input->SetTypeAnn(ctx.MakeType<TListExprType>(outputType));
    return TStatus::Ok;
}

THolder<IGraphTransformer> CreateDqTypeAnnotationTransformer(TTypeAnnotationContext& typesCtx) {
    auto coreTransformer = CreateExtCallableTypeAnnotationTransformer(typesCtx);

    return CreateFunctorTransformer(
        [coreTransformer](const TExprNode::TPtr& input, TExprNode::TPtr& output, TExprContext& ctx) {
            output = input;
            TIssueScopeGuard issueScope(ctx.IssueManager, [&input, &ctx] {
                return MakeIntrusive<TIssue>(ctx.GetPosition(input->Pos()),
                    TStringBuilder() << "At function: " << input->Content());
            });

            if (TDqStage::Match(input.Get())) {
                return AnnotateDqStage(input, ctx);
            }

            if (TDqPhyStage::Match(input.Get())) {
                return AnnotateDqPhyStage(input, ctx);
            }

            if (TDqOutput::Match(input.Get())) {
                return AnnotateDqOutput(input, ctx);
            }

            if (TDqCnUnionAll::Match(input.Get())) {
                return AnnotateDqConnection(input, ctx);
            }

            if (TDqCnHashShuffle::Match(input.Get())) {
                return AnnotateDqCnHashShuffle(input, ctx);
            }

            if (TDqCnMap::Match(input.Get())) {
                return AnnotateDqConnection(input, ctx);
            }

            if (TDqCnBroadcast::Match(input.Get())) {
                return AnnotateDqConnection(input, ctx);
            }

            if (TDqCnResult::Match(input.Get())) {
                return AnnotateDqCnResult(input, ctx);
            }

            if (TDqCnValue::Match(input.Get())) {
                return AnnotateDqCnValue(input, ctx);
            }

            if (TDqCnMerge::Match(input.Get())) {
                return AnnotateDqCnMerge(input, ctx);
            }

            if (TDqReplicate::Match(input.Get())) {
                return AnnotateDqReplicate(input, ctx);
            }

            if (TDqJoin::Match(input.Get())) {
                return AnnotateDqJoin(input, ctx);
            }

            if (TDqPhyMapJoin::Match(input.Get())) {
                return AnnotateDqMapOrDictJoin(input, ctx);
            }

            if (TDqPhyJoinDict::Match(input.Get())) {
                return AnnotateDqMapOrDictJoin(input, ctx);
            }

            if (TDqPhyCrossJoin::Match(input.Get())) {
                return AnnotateDqCrossJoin(input, ctx);
            }

            if (TDqSource::Match(input.Get())) {
                return AnnotateDqSource(input, ctx);
            }

            if (TDqSink::Match(input.Get())) {
                return AnnotateDqSink(input, ctx);
            }

            if (TTransformSettings::Match(input.Get())) {
                return AnnotateTransformSettings (input, ctx);
            }

            if (TDqQuery::Match(input.Get())) {
                return AnnotateDqQuery(input, ctx);
            }

            if (TDqPrecompute::Match(input.Get())) {
                return AnnotateDqPrecompute(input, ctx);
            }

            if (TDqPhyPrecompute::Match(input.Get())) {
                return AnnotateDqPhyPrecompute(input, ctx);
            }

            return coreTransformer->Transform(input, output, ctx);
        });
}

bool IsTypeSupportedInMergeCn(EDataSlot type) {
    switch (type) {
        case EDataSlot::Bool:
        case EDataSlot::Int8:
        case EDataSlot::Uint8:
        case EDataSlot::Int16:
        case EDataSlot::Uint16:
        case EDataSlot::Int32:
        case EDataSlot::Uint32:
        case EDataSlot::Int64:
        case EDataSlot::Uint64:
        case EDataSlot::Double:
        case EDataSlot::Float:
        case EDataSlot::String:
        case EDataSlot::Utf8:
        case EDataSlot::Uuid:
        case EDataSlot::Date:
        case EDataSlot::Datetime:
        case EDataSlot::Timestamp:
        case EDataSlot::Interval:
        case EDataSlot::Decimal:
        case EDataSlot::DyNumber:
            // Supported
            return true;
        case EDataSlot::Yson:
        case EDataSlot::Json:
        case EDataSlot::TzDate:
        case EDataSlot::TzDatetime:
        case EDataSlot::TzTimestamp:
        case EDataSlot::JsonDocument:
            return false;
    }
    return false;
}

bool IsTypeSupportedInMergeCn(const TDataExprType* dataType) {
   return IsTypeSupportedInMergeCn(dataType->GetSlot());
}

bool IsMergeConnectionApplicable(const TVector<const TTypeAnnotationNode*>& sortKeyTypes) {
    for (auto sortKeyType : sortKeyTypes) {
        if (sortKeyType->GetKind() == ETypeAnnotationKind::Optional) {
            sortKeyType = sortKeyType->Cast<TOptionalExprType>()->GetItemType();
        }
        if (sortKeyType->GetKind() != ETypeAnnotationKind::Data
            || !IsTypeSupportedInMergeCn(sortKeyType->Cast<TDataExprType>())) {
            return false;
        }
    }
    return true;
}

TString PrintDqStageOnly(const TDqStageBase& stage, TExprContext& ctx) {
    if (stage.Inputs().Empty()) {
        return NCommon::ExprToPrettyString(ctx, stage.Ref());
    }

    TNodeOnNodeOwnedMap replaces;
    for (ui64 i = 0; i < stage.Inputs().Size(); ++i) {
        auto input = stage.Inputs().Item(i);
        auto param = Build<NNodes::TCoParameter>(ctx, input.Pos())
            .Name().Build(TStringBuilder() << "stage_input_" << i)
            .Type(ExpandType(input.Pos(), *input.Ref().GetTypeAnn(), ctx))
            .Done();

        replaces[input.Raw()] = param.Ptr();
    }

    auto newStage = ctx.ReplaceNodes(stage.Ptr(), replaces);
    return NCommon::ExprToPrettyString(ctx, *newStage);
}

} // namespace NYql::NDq
