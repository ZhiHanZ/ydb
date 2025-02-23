#pragma once

#include "defs.h"

#include <ydb/core/kqp/runtime/kqp_tasks_runner.h>
#include <ydb/core/tablet_flat/tablet_flat_executor.h>
#include <ydb/core/engine/mkql_engine_flat.h>
#include <ydb/core/engine/minikql/minikql_engine_host.h>
#include <ydb/core/tx/datashard/datashard_kqp_compute.h>

namespace NKikimr {

namespace NTabletFlatExecutor {
    class TTransactionContext;
}

using NTabletFlatExecutor::TTransactionContext;

namespace NDataShard {

class TDataShard;

TIntrusivePtr<TThrRefBase> InitDataShardSysTables(TDataShard* self);

///
class TEngineBay : TNonCopyable {
public:
    using TValidationInfo = NMiniKQL::IEngineFlat::TValidationInfo;
    using TValidatedKey = NMiniKQL::IEngineFlat::TValidatedKey;
    using EResult = NMiniKQL::IEngineFlat::EResult;
    using TEngineHostCounters = NMiniKQL::TEngineHostCounters;

    struct TSizes {
        ui64 ReadSize = 0;
        ui64 ReplySize = 0;
        THashMap<ui64, ui64> OutReadSetSize;
        ui64 TotalKeysSize = 0;
    };

    TEngineBay(TDataShard * self, TTransactionContext& txc, const TActorContext& ctx,
               std::pair<ui64, ui64> stepTxId);

    virtual ~TEngineBay();

    const NMiniKQL::IEngineFlat * GetEngine() const { return Engine.Get(); }
    NMiniKQL::IEngineFlat * GetEngine();
    void SetLockTxId(ui64 lockTxId);
    void SetUseLlvmRuntime(bool llvmRuntime) { EngineSettings->LlvmRuntime = llvmRuntime; }

    EResult Validate() {
        if (Info.Loaded)
            return EResult::Ok;
        Y_VERIFY(Engine);
        return Engine->Validate(Info);
    }

    EResult ReValidateKeys() {
        Y_VERIFY(Info.Loaded);
        Y_VERIFY(Engine);
        return Engine->ValidateKeys(Info);
    }

    void AddReadRange(const TTableId& tableId, const TVector<NTable::TColumn>& columns, const TTableRange& range,
                      const TVector<NScheme::TTypeId>& keyTypes, ui64 itemsLimit = 0, bool reverse = false);

    void AddWriteRange(const TTableId& tableId, const TTableRange& range, const TVector<NScheme::TTypeId>& keyTypes);

    void MarkTxLoaded() {
        Info.Loaded = true;
    }

    /// @note it expects TValidationInfo keys are materialized outsize of engine's allocs
    void DestroyEngine() {
        ComputeCtx->Clear();
        if (KqpTasksRunner) {
            KqpTasksRunner.Reset();
            {
                auto guard = TGuard(*KqpAlloc);
                KqpTypeEnv.Reset();
            }
            KqpAlloc.Reset();
        }
        KqpExecCtx = {};

        Engine.Reset();
        EngineHost.Reset();
        TraceMessage.clear();
    }

    const TValidationInfo& TxInfo() const { return Info; }
    TEngineBay::TSizes CalcSizes(bool needsTotalKeysSize) const;

    void SetWriteVersion(TRowVersion writeVersion);
    void SetReadVersion(TRowVersion readVersion);
    void SetIsImmediateTx();

    TVector<NMiniKQL::IChangeCollector::TChange> GetCollectedChanges() const;

    void ResetCounters() { EngineHostCounters = TEngineHostCounters(); }
    const TEngineHostCounters& GetCounters() const { return EngineHostCounters; }

    NKqp::TKqpTasksRunner& GetKqpTasksRunner(const NKikimrTxDataShard::TKqpTransaction& tx);
    NMiniKQL::TKqpDatashardComputeContext& GetKqpComputeCtx();

private:
    std::pair<ui64, ui64> StepTxId;
    THolder<NMiniKQL::TEngineHost> EngineHost;
    THolder<NMiniKQL::TEngineFlatSettings> EngineSettings;
    THolder<NMiniKQL::IEngineFlat> Engine;
    TValidationInfo Info;
    TEngineHostCounters EngineHostCounters;
    ui64 LockTxId;
    TString TraceMessage;
    NYql::NDq::TLogFunc KqpLogFunc;
    THolder<NUdf::IApplyContext> KqpApplyCtx;
    THolder<NMiniKQL::TKqpDatashardComputeContext> ComputeCtx;
    THolder<NMiniKQL::TScopedAlloc> KqpAlloc;
    THolder<NMiniKQL::TTypeEnvironment> KqpTypeEnv;
    NYql::NDq::TDqTaskRunnerContext KqpExecCtx;
    TIntrusivePtr<NKqp::TKqpTasksRunner> KqpTasksRunner;
};

}}
