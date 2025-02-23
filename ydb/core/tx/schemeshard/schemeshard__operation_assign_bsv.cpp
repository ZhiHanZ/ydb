#include "schemeshard__operation_part.h"
#include "schemeshard_impl.h"

namespace {

using namespace NKikimr;
using namespace NSchemeShard;

class TAssignBlockStoreVolume: public ISubOperationBase {
    const TOperationId OperationId;
    const TTxTransaction Transaction;

public:
    TAssignBlockStoreVolume(TOperationId id, const TTxTransaction& tx)
        : OperationId(id)
        , Transaction(tx)
    {
    }

    TAssignBlockStoreVolume(TOperationId id)
        : OperationId(id)
    {
    }

    THolder<TProposeResponse> Propose(const TString&, TOperationContext& context) override {
        const TTabletId ssId = context.SS->SelfTabletId();

        const TString& parentPathStr = Transaction.GetWorkingDir();
        const TString& name = Transaction.GetAssignBlockStoreVolume().GetName();
        const TString mountToken = Transaction.GetAssignBlockStoreVolume().GetNewMountToken();
        const auto version = Transaction.GetAssignBlockStoreVolume().GetTokenVersion();

        LOG_NOTICE_S(context.Ctx, NKikimrServices::FLAT_TX_SCHEMESHARD,
                     "TAssignBlockStoreVolume Propose"
                         << ", path: " << parentPathStr << "/" << name
                         << ", operationId: " << OperationId
                         << ", at schemeshard: " << ssId);

        auto result = MakeHolder<TProposeResponse>(NKikimrScheme::StatusSuccess, ui64(OperationId.GetTxId()), context.SS->TabletID());

        TPath path = TPath::Resolve(parentPathStr, context.SS).Dive(name);

        {
            TPath::TChecker checks = path.Check();
            checks
                .NotEmpty()
                .NotUnderDomainUpgrade()
                .IsAtLocalSchemeShard()
                .IsResolved()
                .NotDeleted()
                .IsBlockStoreVolume()
                .IsCommonSensePath(); //forbid alter impl index tables

            if (!checks) {
                TString explain = TStringBuilder() << "path fail checks"
                                                   << ", path: " << path.PathString();
                auto status = checks.GetStatus(&explain);
                result->SetError(status, explain);
                return result;
            }
        }

        if (!context.SS->BlockStoreVolumes.contains(path.Base()->PathId)) {
            result->SetError(NKikimrScheme::StatusInvalidParameter, "Specified path is not a block store volume");
            return result;
        }

        TBlockStoreVolumeInfo::TPtr volume = context.SS->BlockStoreVolumes[path.Base()->PathId];
        if (volume->AlterVersion == 0) {
            result->SetError(NKikimrScheme::StatusMultipleModifications, "Block store volume is not created yet");
            return result;
        }


        if (version &&
            version != volume->TokenVersion)
        {
            result->SetError(
                NKikimrScheme::StatusPreconditionFailed,
                "Wrong version in Assign Volume");
            return result;
        }

        TString errStr;
        if (!context.SS->CheckApplyIf(Transaction, errStr)) {
            result->SetError(NKikimrScheme::StatusPreconditionFailed, errStr);
            return result;
        }

        NIceDb::TNiceDb db(context.Txc.DB);

        volume->MountToken = mountToken;
        ++volume->TokenVersion;
        context.SS->PersistBlockStoreVolumeMountToken(db, path.Base()->PathId, volume);

        context.OnComplete.PublishToSchemeBoard(OperationId, path.Base()->PathId);

        context.OnComplete.DoneOperation(OperationId);
        return result;
    }

    void AbortPropose(TOperationContext&) override {
        Y_FAIL("no AbortPropose for TAssignBlockStoreVolume");
    }

    void ProgressState(TOperationContext&) override {
        Y_FAIL("no progress state for assign bsc");
        return;
    }

    void AbortUnsafe(TTxId, TOperationContext&) override {
        Y_FAIL("no AbortUnsafe for assign bsc");
    }
};

}

namespace NKikimr {
namespace NSchemeShard {

ISubOperationBase::TPtr CreateAssignBSV(TOperationId id, const TTxTransaction& tx) {
    return new TAssignBlockStoreVolume(id, tx);
}

ISubOperationBase::TPtr CreateAssignBSV(TOperationId id, TTxState::ETxState state) {
    Y_VERIFY(state == TTxState::Invalid || state == TTxState::Propose);
    return new TAssignBlockStoreVolume(id);
}

}
}
