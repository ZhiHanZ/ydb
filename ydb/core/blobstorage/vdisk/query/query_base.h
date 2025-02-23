#pragma once
#include "defs.h"
#include "query_readbatch.h"
#include "query_public.h"
#include "query_spacetracker.h"
#include <ydb/core/blobstorage/vdisk/hulldb/hull_ds_all_snap.h>
#include <ydb/core/blobstorage/vdisk/common/vdisk_response.h>
#include <ydb/core/blobstorage/base/wilson_events.h>

namespace NKikimr {

    ////////////////////////////////////////////////////////////////////////////
    // TLevelIndexQueryBase
    ////////////////////////////////////////////////////////////////////////////
    class TLevelIndexQueryBase {
    protected:
        std::shared_ptr<TQueryCtx> QueryCtx;
        const TActorId ParentId;
        TLogoBlobsSnapshot LogoBlobsSnapshot;
        TBarriersSnapshot BarriersSnapshot;
        TReadBatcherCtxPtr BatcherCtx;
        const NKikimrBlobStorage::TEvVGet &Record;
        const bool ShowInternals;
        std::unique_ptr<TEvBlobStorage::TEvVGetResult> Result;
        TQueryResultSizeTracker ResultSize;
        const TActorId ReplSchedulerId;

        TLevelIndexQueryBase(
                std::shared_ptr<TQueryCtx> &queryCtx,
                const TActorId &parentId,
                TLogoBlobsSnapshot &&logoBlobsSnapshot,
                TBarriersSnapshot &&barrierSnapshot,
                TEvBlobStorage::TEvVGet::TPtr &ev,
                std::unique_ptr<TEvBlobStorage::TEvVGetResult> result,
                TActorId replSchedulerId)
            : QueryCtx(queryCtx)
            , ParentId(parentId)
            , LogoBlobsSnapshot(std::move(logoBlobsSnapshot))
            , BarriersSnapshot(std::move(barrierSnapshot))
            , BatcherCtx(new TReadBatcherCtx(QueryCtx->HullCtx->VCtx, QueryCtx->PDiskCtx, ev))
            , Record(BatcherCtx->OrigEv->Get()->Record)
            , ShowInternals(Record.GetShowInternals())
            , Result(std::move(result))
            , ReplSchedulerId(replSchedulerId)
        {
            Y_VERIFY_DEBUG(Result);
        }

        ui8 PDiskPriority() const {
            ui8 priority = 0;
            Y_VERIFY(Record.HasHandleClass());
            switch (Record.GetHandleClass()) {
                case NKikimrBlobStorage::EGetHandleClass::AsyncRead:
                    priority = NPriRead::HullOnlineOther;
                    break;
                case NKikimrBlobStorage::EGetHandleClass::FastRead:
                case NKikimrBlobStorage::EGetHandleClass::Discover:
                    priority = NPriRead::HullOnlineRt;
                    break;
                case NKikimrBlobStorage::EGetHandleClass::LowRead:
                    priority = NPriRead::HullLow;
                    break;
                default:
                    Y_FAIL("Unexpected case");
            }
            return priority;
        }

        bool IsRepl() const {
            return Result->Record.HasMsgQoS()
                ? NBackpressure::TQueueClientId(Result->Record.GetMsgQoS()).IsRepl()
                : false;
        }

        template <class T>
        void SendResponseAndDie(const TActorContext &ctx, T *self) {
            bool hasNotYet = false;

            if (ResultSize.IsOverflow()) {
                Result->Record.SetStatus(NKikimrProto::ERROR);
                Result->Record.MutableResult()->Clear();
                // for every 'extreme' query add ERROR result, for 'range' would empty vec for now
                if (Record.ExtremeQueriesSize() > 0) {
                    for (size_t i = 0; i < Record.ExtremeQueriesSize(); ++i) {
                        const auto &r = Record.GetExtremeQueries(i);
                        auto id = LogoBlobIDFromLogoBlobID(r.GetId());
                        ui64 cookie = r.GetCookie();
                        Result->AddResult(NKikimrProto::ERROR, id, &cookie);
                    }
                }
                LOG_CRIT(ctx, NKikimrServices::BS_VDISK_GET,
                        VDISKP(QueryCtx->HullCtx->VCtx->VDiskLogPrefix,
                            "TEvVGetResult: Result message is too large; size# %" PRIu64 " orig# %s;"
                            " VDISK CAN NOT REPLY ON TEvVGet REQUEST",
                            ResultSize.GetSize(), BatcherCtx->OrigEv->Get()->ToString().data()));
            } else {
                ui64 total = 0;
                for (const auto& result : Result->Record.GetResult()) {
                    total += result.GetBuffer().size();
                    hasNotYet = hasNotYet || result.GetStatus() == NKikimrProto::NOT_YET;
                }
                QueryCtx->MonGroup.GetTotalBytes() += total;
                LOG_DEBUG(ctx, NKikimrServices::BS_VDISK_GET,
                        VDISKP(QueryCtx->HullCtx->VCtx->VDiskLogPrefix,
                            "TEvVGetResult: %s", Result->ToString().data()));
            }

            if (hasNotYet && ReplSchedulerId) {
                // send reply event to repl scheduler to possibly fix NOT_YET replies
                ctx.Send(ReplSchedulerId, new TEvBlobStorage::TEvEnrichNotYet(BatcherCtx->OrigEv, std::move(Result)));
            } else {
                // send reply event to sender
                SendVDiskResponse(ctx, BatcherCtx->OrigEv->Sender, Result.release(), *self, BatcherCtx->OrigEv->Cookie);
            }

            ctx.Send(ParentId, new TEvents::TEvActorDied);
            self->Die(ctx);
        }
    };

} // NKikimr
