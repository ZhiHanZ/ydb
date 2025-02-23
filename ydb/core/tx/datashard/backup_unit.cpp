#include "export_iface.h"
#include "backup_restore_common.h"
#include "execution_unit_ctors.h"
#include "export_scan.h"
#include "export_s3.h"

namespace NKikimr {
namespace NDataShard {

class TBackupUnit : public TBackupRestoreUnitBase<TEvDataShard::TEvCancelBackup> {
    using IBuffer = NExportScan::IBuffer;

protected:
    bool IsRelevant(TActiveTransaction* tx) const override {
        return tx->GetSchemeTx().HasBackup();
    }

    bool IsWaiting(TOperation::TPtr op) const override {
        return op->IsWaitingForScan();
    }

    void SetWaiting(TOperation::TPtr op) override {
        op->SetWaitingForScanFlag();
    }

    void ResetWaiting(TOperation::TPtr op) override {
        op->ResetWaitingForScanFlag();
    }

    bool Run(TOperation::TPtr op, TTransactionContext& txc, const TActorContext& ctx) override {
        TActiveTransaction* tx = dynamic_cast<TActiveTransaction*>(op.Get());
        Y_VERIFY_S(tx, "cannot cast operation of kind " << op->GetKind());

        Y_VERIFY(tx->GetSchemeTx().HasBackup());
        const auto& backup = tx->GetSchemeTx().GetBackup();

        const ui64 tableId = backup.GetTableId();
        Y_VERIFY(DataShard.GetUserTables().contains(tableId));

        const ui32 localTableId = DataShard.GetUserTables().at(tableId)->LocalTid;
        Y_VERIFY(txc.DB.GetScheme().GetTableInfo(localTableId));

        auto* appData = AppData(ctx);

        std::shared_ptr<::NKikimr::NDataShard::IExport> exp;
        if (backup.HasYTSettings()) {
            auto* exportFactory = appData->DataShardExportFactory;
            if (exportFactory) {
                const auto& settings = backup.GetYTSettings();
                std::shared_ptr<IExport>(exportFactory->CreateExportToYt(settings.GetUseTypeV3())).swap(exp);
            } else {
                Abort(op, ctx, "Exports to YT are disabled");
                return false;
            }
        } else if (backup.HasS3Settings()) {
            auto* exportFactory = appData->DataShardExportFactory;
            if (exportFactory) {
                std::shared_ptr<IExport>(exportFactory->CreateExportToS3()).swap(exp);
            } else {
                Abort(op, ctx, "Exports to S3 are disabled");
                return false;
            }
        } else {
            Abort(op, ctx, "Unsupported backup task");
            return false;
        }

        const auto& columns = DataShard.GetUserTables().at(tableId)->Columns;
        const auto& scanSettings = backup.GetScanSettings();
        const ui64 rowsLimit = scanSettings.GetRowsBatchSize() ? scanSettings.GetRowsBatchSize() : Max<ui64>();
        const ui64 bytesLimit = scanSettings.GetBytesBatchSize();

        auto createUploader = [self = DataShard.SelfId(), txId = op->GetTxId(), columns, backup, exp]() {
            return exp->CreateUploader(self, txId, columns, backup);
        };

        THolder<IBuffer> buffer{exp->CreateBuffer(columns, rowsLimit, bytesLimit)};
        THolder<NTable::IScan> scan{CreateExportScan(std::move(buffer), createUploader)};

        const auto& taskName = appData->DataShardConfig.GetBackupTaskName();
        const auto taskPrio = appData->DataShardConfig.GetBackupTaskPriority();

        ui64 readAheadLo = appData->DataShardConfig.GetBackupReadAheadLo();
        if (ui64 readAheadLoOverride = DataShard.GetBackupReadAheadLoOverride(); readAheadLoOverride > 0) {
            readAheadLo = readAheadLoOverride;
        }

        ui64 readAheadHi = appData->DataShardConfig.GetBackupReadAheadHi();
        if (ui64 readAheadHiOverride = DataShard.GetBackupReadAheadHiOverride(); readAheadHiOverride > 0) {
            readAheadHi = readAheadHiOverride;
        }

        tx->SetScanTask(DataShard.QueueScan(localTableId, scan.Release(), op->GetTxId(),
            TScanOptions()
                .SetResourceBroker(taskName, taskPrio)
                .SetReadAhead(readAheadLo, readAheadHi)
                .SetReadPrio(TScanOptions::EReadPrio::Low)
        ));

        return true;
    }

    bool HasResult(TOperation::TPtr op) const override {
        return op->HasScanResult();
    }

    void ProcessResult(TOperation::TPtr op, const TActorContext&) override {
        TActiveTransaction* tx = dynamic_cast<TActiveTransaction*>(op.Get());
        Y_VERIFY_S(tx, "cannot cast operation of kind " << op->GetKind());

        auto* result = CheckedCast<TExportScanProduct*>(op->ScanResult().Get());
        auto* schemeOp = DataShard.FindSchemaTx(op->GetTxId());

        schemeOp->Success = result->Success;
        schemeOp->Error = std::move(result->Error);
        schemeOp->BytesProcessed = result->BytesRead;
        schemeOp->RowsProcessed = result->RowsRead;

        op->SetScanResult(nullptr);
        tx->SetScanTask(0);
    }

    void Cancel(TActiveTransaction* tx, const TActorContext&) override {
        if (!tx->GetScanTask()) {
            return;
        }

        const ui64 tableId = tx->GetSchemeTx().GetBackup().GetTableId();

        Y_VERIFY(DataShard.GetUserTables().contains(tableId));
        const ui32 localTableId = DataShard.GetUserTables().at(tableId)->LocalTid;

        DataShard.CancelScan(localTableId, tx->GetScanTask());
        tx->SetScanTask(0);
    }

public:
    TBackupUnit(TDataShard& self, TPipeline& pipeline)
        : TBase(EExecutionUnitKind::Backup, self, pipeline)
    {
    }

}; // TBackupUnit

THolder<TExecutionUnit> CreateBackupUnit(TDataShard& self, TPipeline& pipeline) {
    return THolder(new TBackupUnit(self, pipeline));
}

} // NDataShard
} // NKikimr
