#pragma once

#include "schemeshard.h"
#include "schemeshard_export.h"
#include "schemeshard_import.h"
#include "schemeshard_build_index.h"
#include "schemeshard_private.h"
#include "schemeshard_types.h"
#include "schemeshard_path_element.h"
#include "schemeshard_path.h"
#include "schemeshard_domain_links.h"
#include "schemeshard_info_types.h"
#include "schemeshard_tx_infly.h"
#include "schemeshard_utils.h"
#include "schemeshard_schema.h"

#include "schemeshard__operation.h"

#include "operation_queue_timer.h"

#include <ydb/core/base/hive.h>
#include <ydb/core/base/storage_pools.h>
#include <ydb/core/base/subdomain.h>
#include <ydb/core/base/tx_processing.h>
#include <ydb/core/cms/console/configs_dispatcher.h>
#include <ydb/core/cms/console/console.h>
#include <ydb/core/kesus/tablet/events.h>
#include <ydb/core/persqueue/events/global.h>
#include <ydb/core/protos/blockstore_config.pb.h>
#include <ydb/core/protos/counters_schemeshard.pb.h>
#include <ydb/core/protos/filestore_config.pb.h>
#include <ydb/core/protos/flat_scheme_op.pb.h>
#include <ydb/core/sys_view/common/events.h>
#include <ydb/core/tablet/pipe_tracker.h>
#include <ydb/core/tablet/tablet_counters.h>
#include <ydb/core/tablet/tablet_pipe_client_cache.h>
#include <ydb/core/tablet_flat/flat_cxx_database.h>
#include <ydb/core/tablet_flat/flat_dbase_scheme.h>
#include <ydb/core/tablet_flat/tablet_flat_executed.h>
#include <ydb/core/tx/message_seqno.h>
#include <ydb/core/tx/scheme_board/events.h>
#include <ydb/core/tx/tx_allocator_client/actor_client.h>
#include <ydb/core/tx/replication/controller/public_events.h>
#include <ydb/core/tx/sequenceshard/public/events.h>
#include <ydb/core/tx/tx_processing.h>
#include <ydb/core/util/pb.h>

#include <ydb/core/blockstore/core/blockstore.h>
#include <ydb/core/filestore/core/filestore.h>

#include <ydb/library/login/login.h>

#include <util/generic/ptr.h>

namespace NKikimr {
namespace NSchemeShard {

extern const ui64 NEW_TABLE_ALTER_VERSION;

class TSchemeShard
    : public TActor<TSchemeShard>
    , public NTabletFlatExecutor::TTabletExecutedFlat
    , public IQuotaCounters
{
private:
    class TPipeClientFactory : public NTabletPipe::IClientFactory {
    public:
        TPipeClientFactory(TSchemeShard* self)
            : Self(self)
        { }

        TActorId CreateClient(const TActorContext& ctx, ui64 tabletId, const NTabletPipe::TClientConfig& pipeConfig) override;

    private:
        TSchemeShard* Self;
    };

    using TCompactionQueue = NOperationQueue::TOperationQueueWithTimer<
        TShardCompactionInfo,
        TCompactionQueueImpl,
        TEvPrivate::EvRunBackgroundCompaction,
        NKikimrServices::FLAT_TX_SCHEMESHARD>;

    class TCompactionStarter : public TCompactionQueue::IStarter {
    public:
        TCompactionStarter(TSchemeShard* self)
            : Self(self)
        { }

        NOperationQueue::EStartStatus StartOperation(const TShardCompactionInfo& info) override {
            return Self->StartBackgroundCompaction(info);
        }

        void OnTimeout(const TShardCompactionInfo& info) override {
            Self->OnBackgroundCompactionTimeout(info);
        }

    private:
        TSchemeShard* Self;
    };

public:
    static constexpr ui32 DefaultPQTabletPartitionsCount = 1;
    static constexpr ui32 MaxPQTabletPartitionsCount = 1000;
    static constexpr ui32 MaxPQGroupTabletsCount = 10*1000;
    static constexpr ui32 MaxPQGroupPartitionsCount = 20*1000;
    static constexpr ui32 MaxPQWriteSpeedPerPartition = 50*1024*1024;
    static constexpr ui32 MaxPQLifetimeSeconds = 31 * 86400;
    static constexpr ui32 PublishChunkSize = 1000;

    static const TSchemeLimits DefaultLimits;

    TIntrusivePtr<TChannelProfiles> ChannelProfiles;

    TControlWrapper AllowConditionalEraseOperations;
    TControlWrapper AllowServerlessStorageBilling;

    TSplitSettings SplitSettings;

    struct TTenantInitState {
        enum EInitState {
            InvalidState = 0,
            Uninitialized = 1,
            Inprogress = 2,
            ReadOnlyPreview = 50,
            Done = 100,
        };
    };
    TTenantInitState::EInitState InitState = TTenantInitState::InvalidState;

    // In RO mode we don't accept any modifications from users but process all in-flight operations in normal way
    bool IsReadOnlyMode = false;

    bool IsDomainSchemeShard = false;

    TPathId ParentDomainId = InvalidPathId;
    TString ParentDomainEffectiveACL;
    ui64 ParentDomainEffectiveACLVersion = 0;
    TEffectiveACL ParentDomainCachedEffectiveACL;
    TString ParentDomainOwner;

    THashSet<TString> SystemBackupSIDs;
    TInstant ServerlessStorageLastBillTime;

    TParentDomainLink ParentDomainLink;
    TSubDomainsLinks SubDomainsLinks;

    TVector<TString> RootPathElements;

    THashMap<TPathId, TPathElement::TPtr> PathsById;
    TLocalPathId NextLocalPathId = 0;

    THashMap<TPathId, TTableInfo::TPtr> Tables;
    THashMap<TPathId, TTableInfo::TPtr> TTLEnabledTables;

    THashMap<TPathId, TTableIndexInfo::TPtr> Indexes;
    THashMap<TPathId, TCdcStreamInfo::TPtr> CdcStreams;
    THashMap<TPathId, TSequenceInfo::TPtr> Sequences;
    THashMap<TPathId, TReplicationInfo::TPtr> Replications;

    THashMap<TPathId, TTxId> TablesWithSnaphots;
    THashMap<TTxId, TSet<TPathId>> SnapshotTables;
    THashMap<TTxId, TStepId> SnapshotsStepIds;

    THashMap<TPathId, TTxId> LockedPaths;

    THashMap<TPathId, TPersQueueGroupInfo::TPtr> PersQueueGroups;
    THashMap<TPathId, TRtmrVolumeInfo::TPtr> RtmrVolumes;
    THashMap<TPathId, TSolomonVolumeInfo::TPtr> SolomonVolumes;
    THashMap<TPathId, TSubDomainInfo::TPtr> SubDomains;
    THashMap<TPathId, TBlockStoreVolumeInfo::TPtr> BlockStoreVolumes;
    THashMap<TPathId, TFileStoreInfo::TPtr> FileStoreInfos;
    THashMap<TPathId, TKesusInfo::TPtr> KesusInfos;
    THashMap<TPathId, TOlapStoreInfo::TPtr> OlapStores;
    THashMap<TPathId, TOlapTableInfo::TPtr> OlapTables;

    // it is only because we need to manage undo of upgrade subdomain, finally remove it
    THashMap<TPathId, TVector<TTabletId>> RevertedMigrations;

    THashMap<TTxId, TOperation::TPtr> Operations;
    THashMap<TTxId, TPublicationInfo> Publications;
    THashMap<TOperationId, TTxState> TxInFlight;

    ui64 NextLocalShardIdx = 0;
    THashMap<TShardIdx, TShardInfo> ShardInfos;
    THashMap<TShardIdx, TAdoptedShard> AdoptedShards;
    THashMap<TTabletId, TShardIdx> TabletIdToShardIdx;
    THashMap<TShardIdx, TVector<TActorId>> ShardDeletionSubscribers; // for tests

    // in case of integral hists we need to remember what values we have set
    struct TPartitionMetrics {
        ui64 SearchHeight = 0;
        ui64 RowDeletes = 0;
        ui32 HoursSinceFullCompaction = 0;
    };
    THashMap<TShardIdx, TPartitionMetrics> PartitionMetricsMap;

    TActorId SchemeBoardPopulator;

    static constexpr ui32 InitiateCachedTxIdsCount = 100;
    TDeque<TTxId> CachedTxIds;
    TActorId TxAllocatorClient;

    TAutoPtr<NTabletPipe::IClientCache> PipeClientCache;
    TPipeTracker PipeTracker;

    TCompactionStarter CompactionStarter;
    TCompactionQueue* CompactionQueue = nullptr;
    THashSet<TShardIdx> ShardsWithBorrowed; // shards have parts from another shards
    THashSet<TShardIdx> ShardsWithLoaned;   // shards have parts loaned to another shards
    bool EnableBackgroundCompaction = false;
    bool EnableBackgroundCompactionServerless = false;

    TShardDeleter ShardDeleter;

    // Counter-strike stuff
    TTabletCountersBase* TabletCounters = nullptr;
    TAutoPtr<TTabletCountersBase> TabletCountersPtr;

    TAutoPtr<TSelfPinger> SelfPinger;

    TActorId SysPartitionStatsCollector;

    TSet<TPathId> CleanDroppedPathsCandidates;
    TSet<TPathId> CleanDroppedSubDomainsCandidates;
    bool CleanDroppedPathsInFly = false;
    bool CleanDroppedPathsDisabled = true;
    bool CleanDroppedSubDomainsInFly = false;

    TActorId DelayedInitTenantDestination;
    TAutoPtr<TEvSchemeShard::TEvInitTenantSchemeShardResult> DelayedInitTenantReply;

    THolder<TProposeResponse> IgniteOperation(TProposeRequest& request, TOperationContext& context);

    TPathId RootPathId() const {
        return MakeLocalId(TPathElement::RootPathId);
    }

    bool IsRootPathId(const TPathId& pId) const {
        return pId == RootPathId();
    }

    bool IsServerlessDomain(const TPath& domain) const {
        const auto& resourcesDomainId = domain.DomainInfo()->GetResourcesDomainId();
        return !IsDomainSchemeShard && resourcesDomainId && resourcesDomainId != ParentDomainId;
    }

    TPathId MakeLocalId(const TLocalPathId& localPathId) const {
        return TPathId(TabletID(), localPathId);
    }

    TShardIdx MakeLocalId(const TLocalShardIdx& localShardIdx) const {
        return TShardIdx(TabletID(), localShardIdx);
    }

    bool IsLocalId(const TPathId& pathId) const {
        return pathId.OwnerId == TabletID();
    }

    bool IsLocalId(const TShardIdx& shardIdx) const {
        return shardIdx.GetOwnerId() == TabletID();
    }

    TPathId GetCurrentSubDomainPathId() const {
        return RootPathId();
    }

    TPathId PeekNextPathId() const {
        return MakeLocalId(NextLocalPathId);
    }

    TPathId AllocatePathId () {
       TPathId next = PeekNextPathId();
       ++NextLocalPathId;
       return next;
    }

    TTxId GetCachedTxId(const TActorContext& ctx);

    EAttachChildResult AttachChild(TPathElement::TPtr child);
    bool PathIsActive(TPathId pathId) const;

    // Transient sequence number that monotonically increases within SS tablet generation. It is included in events
    // sent from SS to DS and is used for deduplication.
    ui64 SchemeOpRound = 1;
    TMessageSeqNo StartRound(TTxState& state);// For SS -> DS propose events
    TMessageSeqNo NextRound();

    void Clear();
    void BreakTabletAndRestart(const TActorContext& ctx);

    bool IsShemeShardConfigured() const;

    ui64 Generation() const;

    void SubscribeConsoleConfigs(const TActorContext& ctx);
    void ApplyConsoleConfigs(const NKikimrConfig::TAppConfig& appConfig, const TActorContext& ctx);
    void ApplyConsoleConfigs(const NKikimrConfig::TFeatureFlags& featureFlags, const TActorContext& ctx);

    void ConfigureCompactionQueue(
        const NKikimrConfig::TCompactionConfig::TBackgroundCompactionConfig& config,
        const TActorContext &ctx);
    void StartStopCompactionQueue();

    bool ApplyStorageConfig(const TStoragePools& storagePools,
                            const NKikimrSchemeOp::TStorageConfig& storageConfig,
                            TChannelsBindings& channelsBinding,
                            THashMap<TString, ui32>& reverseBinding,
                            TStorageRoom& room,
                            TString& errorMsg);
    bool GetBindingsRooms(const TPathId domainId,
                          const NKikimrSchemeOp::TPartitionConfig& partitionConfig,
                          TVector<TStorageRoom>& rooms,
                          THashMap<ui32, ui32>& familyRooms,
                          TChannelsBindings& binding,
                          TString& errStr);

    /**
     * For each existing partition generates possible changes to channels
     * cand per-shard partition config based on an updated partitionConfig
     * for a table in the given domain.
     */
    bool GetBindingsRoomsChanges(
            const TPathId domainId,
            const TVector<TTableShardInfo>& partitions,
            const NKikimrSchemeOp::TPartitionConfig& partitionConfig,
            TBindingsRoomsChanges& changes,
            TString& errStr);

    /**
     * Generates channels bindings for column shards based on the given storage config
     */
    bool GetOlapChannelsBindings(const TPathId domainId,
                                 const NKikimrSchemeOp::TColumnStorageConfig& channelsConfig,
                                 TChannelsBindings& channelsBindings,
                                 TString& errStr);

    bool IsStorageConfigLogic(const TTableInfo::TCPtr tableInfo) const;
    bool IsCompatibleChannelProfileLogic(const TPathId domainId, const TTableInfo::TCPtr tableInfo) const;
    bool GetChannelsBindings(const TPathId domainId, const TTableInfo::TCPtr tableInfo, TChannelsBindings& binding, TString& errStr)   const;

    bool ResolveTabletChannels(ui32 profileId, const TPathId domainId, TChannelsBindings& channelsBinding) const;
    bool ResolveRtmrChannels(const TPathId domainId, TChannelsBindings& channelsBinding) const;
    bool ResolveSolomonChannels(ui32 profileId, const TPathId domainId, TChannelsBindings& channelsBinding) const;
    bool ResolvePqChannels(ui32 profileId, const TPathId domainId, TChannelsBindings& channelsBinding) const;
    bool ResolveChannelsByPoolKinds(
        const TVector<TStringBuf>& channelPoolKinds,
        const TPathId domainId,
        TChannelsBindings& channelsBinding) const;
    static void SetNbsChannelsParams(
        const google::protobuf::RepeatedPtrField<NKikimrBlockStore::TChannelProfile>& ecps,
        TChannelsBindings& channelsBinding);
    static void SetNfsChannelsParams(
        const google::protobuf::RepeatedPtrField<NKikimrFileStore::TChannelProfile>& ecps,
        TChannelsBindings& channelsBinding);
    static void SetPqChannelsParams(
        const google::protobuf::RepeatedPtrField<NKikimrPQ::TChannelProfile>& ecps,
        TChannelsBindings& channelsBinding);

    bool ResolveSubdomainsChannels(const TStoragePools& storagePools, TChannelsBindings& channelsBinding);

    using TChannelResolveDetails = std::function<bool (ui32 profileId,
                                                      const TChannelProfiles::TProfile& profile,
                                                      const TStoragePools& storagePools,
                                                      TChannelsBindings& channelsBinding)>;
    bool ResolveChannelCommon(ui32 profileId, const TPathId domainId, TChannelsBindings& channelsBinding, TChannelResolveDetails resolveDetails) const;
    static bool ResolveChannelsDetailsAsIs(ui32 /*profileId*/, const TChannelProfiles::TProfile& profile, const TStoragePools& storagePools, TChannelsBindings& channelsBinding);
    static bool TabletResolveChannelsDetails(ui32 profileId, const TChannelProfiles::TProfile& profile, const TStoragePools& storagePools, TChannelsBindings& channelsBinding);

    void ClearDescribePathCaches(const TPathElement::TPtr node);
    TString PathToString(TPathElement::TPtr item);
    NKikimrSchemeOp::TPathVersion  GetPathVersion(const TPath& pathEl) const;

    const TTableInfo* GetMainTableForIndex(TPathId indexTableId) const;

    TPathId ResolveDomainId(TPathId pathId) const;
    TPathId ResolveDomainId(TPathElement::TPtr pathEl) const;
    TSubDomainInfo::TPtr ResolveDomainInfo(TPathId pathId) const;
    TSubDomainInfo::TPtr ResolveDomainInfo(TPathElement::TPtr pathEl) const;

    TPathId GetDomainKey(TPathId pathId) const;

    const NKikimrSubDomains::TProcessingParams& SelectProcessingPrarams(TPathId id) const;
    const NKikimrSubDomains::TProcessingParams& SelectProcessingPrarams(TPathElement::TPtr pathEl) const;

    TTabletId SelectCoordinator(TTxId txId, TPathId pathId) const;
    TTabletId SelectCoordinator(TTxId txId, TPathElement::TPtr pathEl) const;

    bool CheckApplyIf(const NKikimrSchemeOp::TModifyScheme& scheme, TString& errStr);
    bool CheckLocks(const TPathId pathId, const TTxId lockTxId, TString& errStr);
    bool CheckLocks(const TPathId pathId, const NKikimrSchemeOp::TModifyScheme &scheme, TString &errStr);

    TShardIdx ReserveShardIdxs(ui64 count);
    TShardIdx NextShardIdx(const TShardIdx& shardIdx, ui64 inc) const;
    TShardIdx RegisterShardInfo(TShardInfo&& shardInfo);
    TShardIdx RegisterShardInfo(const TShardInfo& shardInfo);
    TShardIdx RegisterShardInfo(const TShardIdx& shardIdx, TShardInfo&& shardInfo);
    TShardIdx RegisterShardInfo(const TShardIdx& shardIdx, const TShardInfo& shardInfo);

    TTxState& CreateTx(TOperationId opId, TTxState::ETxType txType, TPathId targetPath, TPathId sourcePath = InvalidPathId);
    TTxState* FindTx(TOperationId opId);
    void RemoveTx(const TActorContext &ctx, NIceDb::TNiceDb& db, TOperationId opId, TTxState* txState);
    static TPathElement::EPathState CalcPathState(TTxState::ETxType txType, TPathElement::EPathState oldState);

    TMaybe<NKikimrSchemeOp::TPartitionConfig> GetTablePartitionConfigWithAlterData(TPathId pathId) const;
    void DeleteSplitOp(TOperationId txId, TTxState& txState);
    bool ShardIsUnderSplitMergeOp(const TShardIdx& idx) const;

    THashSet<TShardIdx> CollectAllShards(const THashSet<TPathId>& pathes) const;
    void ExamineTreeVFS(TPathId nodeId, std::function<void(TPathElement::TPtr)> func, const TActorContext& ctx);
    THashSet<TPathId> ListSubThee(TPathId subdomain_root, const TActorContext& ctx);
    THashSet<TTxId> GetRelatedTransactions(const THashSet<TPathId>& pathes, const TActorContext &ctx);

    void MarkAsDroping(TPathElement::TPtr node, TTxId txId, const TActorContext& ctx);
    void MarkAsDroping(const THashSet<TPathId>& pathes, TTxId txId, const TActorContext& ctx);

    void UncountNode(TPathElement::TPtr node);
    void MarkAsMigrated(TPathElement::TPtr node, const TActorContext& ctx);

    void DropNode(TPathElement::TPtr node, TStepId step, TTxId txId, NIceDb::TNiceDb& db, const TActorContext& ctx);
    void DropPathes(const THashSet<TPathId>& pathes, TStepId step, TTxId txId, NIceDb::TNiceDb& db, const TActorContext& ctx);

    void DoShardsDeletion(const THashSet<TShardIdx>& shardIdx, const TActorContext& ctx);

    void SetPartitioning(TPathId pathId, TTableInfo::TPtr tableInfo, TVector<TTableShardInfo>&& newPartitioning);
    auto BuildStatsForCollector(TPathId tableId, TShardIdx shardIdx, TTabletId datashardId,
        TMaybe<ui32> nodeId, TMaybe<ui64> startTime, const TTableInfo::TPartitionStats& stats);

    bool ReadSysValue(NIceDb::TNiceDb& db, ui64 sysTag, TString& value, TString defValue = TString());
    bool ReadSysValue(NIceDb::TNiceDb& db, ui64 sysTag, ui64& value, ui64 defVal = 0);

    void IncrementPathDbRefCount(const TPathId& pathId, const TStringBuf& debug = TStringBuf());
    void DecrementPathDbRefCount(const TPathId& pathId, const TStringBuf& debug = TStringBuf());

    // path
    void PersistPath(NIceDb::TNiceDb& db, const TPathId& pathId);
    void PersistRemovePath(NIceDb::TNiceDb& db, const TPathElement::TPtr path);
    void PersistLastTxId(NIceDb::TNiceDb& db, const TPathElement::TPtr path);
    void PersistPathDirAlterVersion(NIceDb::TNiceDb& db, const TPathElement::TPtr path);
    void PersistACL(NIceDb::TNiceDb& db, const TPathElement::TPtr path);
    void PersistOwner(NIceDb::TNiceDb& db, const TPathElement::TPtr path);
    void PersistCreateTxId(NIceDb::TNiceDb& db, const TPathId pathId, TTxId txId);
    void PersistCreateStep(NIceDb::TNiceDb& db, const TPathId pathId, TStepId step);
    void PersistDropStep(NIceDb::TNiceDb& db, const TPathId pathId, TStepId step, TOperationId opId);

    // user attrs
    void ApplyAndPersistUserAttrs(NIceDb::TNiceDb& db, const TPathId& pathId);
    void PersistUserAttributes(NIceDb::TNiceDb& db, TPathId pathId, TUserAttributes::TPtr oldAttrs, TUserAttributes::TPtr alterAttrs);
    void PersistAlterUserAttributes(NIceDb::TNiceDb& db, TPathId pathId);

    // table index
    void PersistTableIndex(NIceDb::TNiceDb& db, const TPathId& pathId);
    void PersistTableIndexAlterData(NIceDb::TNiceDb& db, const TPathId& pathId);

    // cdc stream
    void PersistCdcStream(NIceDb::TNiceDb& db, const TPathId& pathId);
    void PersistCdcStreamAlterData(NIceDb::TNiceDb& db, const TPathId& pathId);
    void PersistRemoveCdcStream(NIceDb::TNiceDb& db, const TPathId& tableId);

    static void PersistTxMinStep(NIceDb::TNiceDb& db, const TOperationId opId, TStepId minStep);
    void PersistRemoveTx(NIceDb::TNiceDb& db, const TOperationId opId, const TTxState& txState);
    void PersistTable(NIceDb::TNiceDb &db, const TPathId pathId);
    void PersistChannelsBinding(NIceDb::TNiceDb& db, const TShardIdx shardId, const TChannelsBindings& bindedChannels);
    void PersistTablePartitioning(NIceDb::TNiceDb &db, const TPathId pathId, const TTableInfo::TPtr tableInfo);
    void DeleteTablePartitioning(NIceDb::TNiceDb& db, const TPathId tableId, const TTableInfo::TPtr tableInfo);
    void PersistTablePartitionCondErase(NIceDb::TNiceDb& db, const TPathId& pathId, ui64 id, const TTableInfo::TPtr tableInfo);
    void PersistTablePartitionStats(NIceDb::TNiceDb& db, const TPathId& tableId, ui64 partitionId, const TTableInfo::TPartitionStats& stats);
    void PersistTablePartitionStats(NIceDb::TNiceDb& db, const TPathId& tableId, const TShardIdx& shardIdx, const TTableInfo::TPtr tableInfo);
    void PersistTablePartitionStats(NIceDb::TNiceDb& db, const TPathId& tableId, const TTableInfo::TPtr tableInfo);
    void PersistTableCreated(NIceDb::TNiceDb& db, const TPathId tableId);
    void PersistTableAlterVersion(NIceDb::TNiceDb &db, const TPathId pathId, const TTableInfo::TPtr tableInfo);
    void PersistTableAltered(NIceDb::TNiceDb &db, const TPathId pathId, const TTableInfo::TPtr tableInfo);
    void PersistAddAlterTable(NIceDb::TNiceDb& db, TPathId pathId, const TTableInfo::TAlterDataPtr alter);
    void PersistPersQueueGroup(NIceDb::TNiceDb &db, TPathId pathId, const TPersQueueGroupInfo::TPtr);
    void PersistRemovePersQueueGroup(NIceDb::TNiceDb &db, TPathId pathId);
    void PersistAddPersQueueGroupAlter(NIceDb::TNiceDb &db, TPathId pathId, const TPersQueueGroupInfo::TPtr);
    void PersistRemovePersQueueGroupAlter(NIceDb::TNiceDb &db, TPathId pathId);
    void PersistPersQueue(NIceDb::TNiceDb &db, TPathId pathId, TShardIdx shardIdx, const TPQShardInfo::TPersQueueInfo& pqInfo);
    void PersistRemovePersQueue(NIceDb::TNiceDb &db, TPathId pathId, ui32 pqId);
    void PersistRtmrVolume(NIceDb::TNiceDb &db, TPathId pathId, const TRtmrVolumeInfo::TPtr rtmrVol);
    void PersistRemoveRtmrVolume(NIceDb::TNiceDb &db, TPathId pathId);
    void PersistSolomonVolume(NIceDb::TNiceDb &db, TPathId pathId, const TSolomonVolumeInfo::TPtr rtmrVol);
    void PersistRemoveSolomonVolume(NIceDb::TNiceDb &db, TPathId pathId);
    void PersistAlterSolomonVolume(NIceDb::TNiceDb &db, TPathId pathId, const TSolomonVolumeInfo::TPtr rtmrVol);
    static void PersistAddTxDependency(NIceDb::TNiceDb& db, const TTxId parentOpId, TTxId txId);
    static void PersistRemoveTxDependency(NIceDb::TNiceDb& db, TTxId opId, TTxId dependentOpId);
    void PersistUpdateTxShard(NIceDb::TNiceDb& db, TOperationId txId, TShardIdx shardIdx, ui32 operation);
    void PersistRemoveTxShard(NIceDb::TNiceDb& db, TOperationId txId, TShardIdx shardIdx);
    void PersistShardMapping(NIceDb::TNiceDb& db, TShardIdx shardIdx, TTabletId tabletId, TPathId pathId, TTxId txId, TTabletTypes::EType type);
    void PersistAdoptedShardMapping(NIceDb::TNiceDb& db, TShardIdx shardIdx, TTabletId tabletId, ui64 prevOwner, TLocalShardIdx prevShardIdx);
    void PersistShardPathId(NIceDb::TNiceDb& db, TShardIdx shardIdx, TPathId pathId);
    void PersistDeleteAdopted(NIceDb::TNiceDb& db, TShardIdx shardIdx);

    void PersistSnapshotTable(NIceDb::TNiceDb& db, const TTxId snapshotId, const TPathId tableId);
    void PersistSnapshotStepId(NIceDb::TNiceDb& db, const TTxId snapshotId, const TStepId stepId);
    void PersistDropSnapshot(NIceDb::TNiceDb& db, const TTxId snapshotId, const TPathId tableId);
    void PersistLongLock(NIceDb::TNiceDb& db, const TTxId lockId, const TPathId pathId);
    void PersistUnLock(NIceDb::TNiceDb& db, const TPathId pathId);

    void PersistTxState(NIceDb::TNiceDb& db, const TOperationId opId);
    void ChangeTxState(NIceDb::TNiceDb& db, const TOperationId opId, TTxState::ETxState newState);
    void PersistCancelTx(NIceDb::TNiceDb& db, const TOperationId opId, const TTxState& txState);
    void PersistTxPlanStep(NIceDb::TNiceDb& db, TOperationId opId, TStepId step);


    void PersistShardTx(NIceDb::TNiceDb& db, TShardIdx shardIdx, TTxId txId);
    void PersistUpdateNextPathId(NIceDb::TNiceDb& db) const;
    void PersistUpdateNextShardIdx(NIceDb::TNiceDb& db) const;
    void PersistParentDomain(NIceDb::TNiceDb& db, TPathId parentDomain) const;
    void PersistParentDomainEffectiveACL(NIceDb::TNiceDb& db, const TString& owner, const TString& effectiveACL, ui64 effectiveACLVersion) const;
    void PersistShardsToDelete(NIceDb::TNiceDb& db, const THashSet<TShardIdx>& shardsIdxs);
    void PersistShardDeleted(NIceDb::TNiceDb& db, TShardIdx shardIdx, const TChannelsBindings& bindedChannels);
    void PersistUnknownShardDeleted(NIceDb::TNiceDb& db, TShardIdx shardIdx);
    void PersistTxShardStatus(NIceDb::TNiceDb& db, TOperationId opId, TShardIdx shardIdx, const TTxState::TShardStatus& status);
    void PersistBackupSettings(NIceDb::TNiceDb& db, TPathId pathId, const NKikimrSchemeOp::TBackupTask& settings);
    void PersistBackupDone(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistCompletedBackupRestore(NIceDb::TNiceDb& db, TTxId txId, const TTxState& txState, const TTableInfo::TBackupRestoreResult& info, TTableInfo::TBackupRestoreResult::EKind kind);
    void PersistCompletedBackup(NIceDb::TNiceDb& db, TTxId txId, const TTxState& txState, const TTableInfo::TBackupRestoreResult& backupInfo);
    void PersistCompletedRestore(NIceDb::TNiceDb& db, TTxId txId, const TTxState& txState, const TTableInfo::TBackupRestoreResult& restoreInfo);
    void PersistSchemeLimit(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistStoragePools(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomain(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistRemoveSubDomain(NIceDb::TNiceDb& db, const TPathId& pathId);
    void PersistSubDomainVersion(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomainAlter(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomainDeclaredSchemeQuotas(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomainDatabaseQuotas(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomainState(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomainSchemeQuotas(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistSubDomainSecurityStateVersion(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistDeleteSubDomainAlter(NIceDb::TNiceDb& db, const TPathId& pathId, const TSubDomainInfo& subDomain);
    void PersistKesusInfo(NIceDb::TNiceDb& db, TPathId pathId, const TKesusInfo::TPtr);
    void PersistKesusVersion(NIceDb::TNiceDb& db, TPathId pathId, const TKesusInfo::TPtr);
    void PersistAddKesusAlter(NIceDb::TNiceDb& db, TPathId pathId, const TKesusInfo::TPtr);
    void PersistRemoveKesusAlter(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistRemoveKesusInfo(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistRemoveTableIndex(NIceDb::TNiceDb& db, TPathId tableId);
    void PersistRemoveTable(NIceDb::TNiceDb& db, TPathId tableId, const TActorContext& ctx);
    void PersistRevertedMirgration(NIceDb::TNiceDb& db, TPathId pathId, TTabletId abandonedSchemeShardId);

    // BlockStore
    void PersistBlockStorePartition(NIceDb::TNiceDb& db, TPathId pathId, ui32 partitionId, TShardIdx shardIdx, ui64 version);
    void PersistBlockStoreVolume(NIceDb::TNiceDb& db, TPathId pathId, const TBlockStoreVolumeInfo::TPtr);
    void PersistBlockStoreVolumeMountToken(NIceDb::TNiceDb& db, TPathId pathId, const TBlockStoreVolumeInfo::TPtr volume);
    void PersistAddBlockStoreVolumeAlter(NIceDb::TNiceDb& db, TPathId pathId, const TBlockStoreVolumeInfo::TPtr);
    void PersistRemoveBlockStoreVolumeAlter(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistRemoveBlockStorePartition(NIceDb::TNiceDb& db, TPathId pathId, ui32 partitionId);
    void PersistRemoveBlockStoreVolume(NIceDb::TNiceDb& db, TPathId pathId);

    // FileStore
    void PersistFileStoreInfo(NIceDb::TNiceDb& db, TPathId pathId, const TFileStoreInfo::TPtr);
    void PersistAddFileStoreAlter(NIceDb::TNiceDb& db, TPathId pathId, const TFileStoreInfo::TPtr);
    void PersistRemoveFileStoreAlter(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistRemoveFileStoreInfo(NIceDb::TNiceDb& db, TPathId pathId);

    // OlapStore
    void PersistOlapStore(NIceDb::TNiceDb& db, TPathId pathId, const TOlapStoreInfo& storeInfo, bool isAlter = false);
    void PersistOlapStoreRemove(NIceDb::TNiceDb& db, TPathId pathId, bool isAlter = false);
    void PersistOlapStoreAlter(NIceDb::TNiceDb& db, TPathId pathId, const TOlapStoreInfo& storeInfo);
    void PersistOlapStoreAlterRemove(NIceDb::TNiceDb& db, TPathId pathId);

    // OlapTable
    void PersistOlapTable(NIceDb::TNiceDb& db, TPathId pathId, const TOlapTableInfo& tableInfo, bool isAlter = false);
    void PersistOlapTableRemove(NIceDb::TNiceDb& db, TPathId pathId, bool isAlter = false);
    void PersistOlapTableAlter(NIceDb::TNiceDb& db, TPathId pathId, const TOlapTableInfo& tableInfo);
    void PersistOlapTableAlterRemove(NIceDb::TNiceDb& db, TPathId pathId);

    // Sequence
    void PersistSequence(NIceDb::TNiceDb& db, TPathId pathId, const TSequenceInfo& sequenceInfo);
    void PersistSequenceRemove(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistSequenceAlter(NIceDb::TNiceDb& db, TPathId pathId, const TSequenceInfo& sequenceInfo);
    void PersistSequenceAlterRemove(NIceDb::TNiceDb& db, TPathId pathId);

    // Replication
    void PersistReplication(NIceDb::TNiceDb& db, TPathId pathId, const TReplicationInfo& replicationInfo);
    void PersistReplicationRemove(NIceDb::TNiceDb& db, TPathId pathId);
    void PersistReplicationAlter(NIceDb::TNiceDb& db, TPathId pathId, const TReplicationInfo& replicationInfo);
    void PersistReplicationAlterRemove(NIceDb::TNiceDb& db, TPathId pathId);

    void PersistAddTableShardPartitionConfig(NIceDb::TNiceDb& db, TShardIdx shardIdx, const NKikimrSchemeOp::TPartitionConfig& config);

    void PersistPublishingPath(NIceDb::TNiceDb& db, TTxId txId, TPathId pathId, ui64 version);
    void PersistRemovePublishingPath(NIceDb::TNiceDb& db, TTxId txId, TPathId pathId, ui64 version);


    void PersistInitState(NIceDb::TNiceDb& db);

    void PersistStorageBillingTime(NIceDb::TNiceDb& db);

    TTabletId GetGlobalHive(const TActorContext& ctx) const;
    TTabletId ResolveHive(TPathId pathId, const TActorContext& ctx) const;
    TTabletId ResolveHive(TShardIdx shardIdx, const TActorContext& ctx) const;
    TShardIdx GetShardIdx(TTabletId tabletId) const;
    TShardIdx MustGetShardIdx(TTabletId tabletId) const;
    TTabletTypes::EType GetTabletType(TTabletId tabletId) const;

    struct TTxMonitoring;
    //OnRenderAppHtmlPage

    struct TTxInit;
    NTabletFlatExecutor::ITransaction* CreateTxInit();

    struct TTxInitRoot;
    NTabletFlatExecutor::ITransaction* CreateTxInitRoot();

    struct TTxInitRootCompatibility;
    NTabletFlatExecutor::ITransaction* CreateTxInitRootCompatibility(TEvSchemeShard::TEvInitRootShard::TPtr &ev);

    struct TTxInitTenantSchemeShard;
    NTabletFlatExecutor::ITransaction* CreateTxInitTenantSchemeShard(TEvSchemeShard::TEvInitTenantSchemeShard::TPtr &ev);

    void ActivateAfterInitialization(const TActorContext &ctx,
                                     TSideEffects::TPublications&& delayPublications = {},
                                     const TVector<ui64>& exportIds = {},
                                     const TVector<ui64>& importsIds = {},
                                     TVector<TPathId>&& tablesToClean = {},
                                     TDeque<TPathId>&& blockStoreVolumesToClean = {}
                                     );

    struct TTxInitPopulator;
    NTabletFlatExecutor::ITransaction* CreateTxInitPopulator(TSideEffects::TPublications&& publications);

    struct TTxInitSchema;
    NTabletFlatExecutor::ITransaction* CreateTxInitSchema();

    struct TTxUpgradeSchema;
    NTabletFlatExecutor::ITransaction* CreateTxUpgradeSchema();

    struct TTxCleanTables;
    NTabletFlatExecutor::ITransaction* CreateTxCleanTables(TVector<TPathId> tablesToClean);

    struct TTxCleanBlockStoreVolumes;
    NTabletFlatExecutor::ITransaction* CreateTxCleanBlockStoreVolumes(TDeque<TPathId>&& blockStoreVolumes);

    struct TTxCleanDroppedPaths;
    NTabletFlatExecutor::ITransaction* CreateTxCleanDroppedPaths();

    void ScheduleCleanDroppedPaths();
    void Handle(TEvPrivate::TEvCleanDroppedPaths::TPtr& ev, const TActorContext& ctx);

    void EnqueueCompaction(const TShardIdx& shardIdx, const TTableInfo::TPartitionStats& stats);
    void UpdateCompaction(const TShardIdx& shardIdx, const TTableInfo::TPartitionStats& stats);
    void RemoveCompaction(const TShardIdx& shardIdx);

    void UpdateShardMetrics(const TShardIdx& shardIdx, const TTableInfo::TPartitionStats& newStats);
    void RemoveShardMetrics(const TShardIdx& shardIdx);

    void ShardRemoved(const TShardIdx& shardIdx);

    NOperationQueue::EStartStatus StartBackgroundCompaction(const TShardCompactionInfo& info);
    void OnBackgroundCompactionTimeout(const TShardCompactionInfo& info);
    void UpdateBackgroundCompactionQueueMetrics();

    struct TTxCleanDroppedSubDomains;
    NTabletFlatExecutor::ITransaction* CreateTxCleanDroppedSubDomains();

    void ScheduleCleanDroppedSubDomains();
    void Handle(TEvPrivate::TEvCleanDroppedSubDomains::TPtr& ev, const TActorContext& ctx);

    struct TTxFixBadPaths;
    NTabletFlatExecutor::ITransaction* CreateTxFixBadPaths();

    struct TTxPublishTenantAsReadOnly;
    NTabletFlatExecutor::ITransaction* CreateTxPublishTenantAsReadOnly(TEvSchemeShard::TEvPublishTenantAsReadOnly::TPtr &ev);

    struct TTxPublishTenant;
    NTabletFlatExecutor::ITransaction* CreateTxPublishTenant(TEvSchemeShard::TEvPublishTenant::TPtr &ev);

    struct TTxMigrate;
    NTabletFlatExecutor::ITransaction* CreateTxMigrate(TEvSchemeShard::TEvMigrateSchemeShard::TPtr &ev);

    struct TTxDescribeScheme;
    NTabletFlatExecutor::ITransaction* CreateTxDescribeScheme(TEvSchemeShard::TEvDescribeScheme::TPtr &ev);

    struct TTxNotifyCompletion;
    NTabletFlatExecutor::ITransaction* CreateTxNotifyTxCompletion(TEvSchemeShard::TEvNotifyTxCompletion::TPtr &ev);

    struct TTxDeleteTabletReply;
    NTabletFlatExecutor::ITransaction* CreateTxDeleteTabletReply(TEvHive::TEvDeleteTabletReply::TPtr& ev);

    struct TTxShardStateChanged;
    NTabletFlatExecutor::ITransaction* CreateTxShardStateChanged(TEvDataShard::TEvStateChanged::TPtr& ev);

    struct TTxRunConditionalErase;
    NTabletFlatExecutor::ITransaction* CreateTxRunConditionalErase(TEvPrivate::TEvRunConditionalErase::TPtr& ev);

    struct TTxScheduleConditionalErase;
    NTabletFlatExecutor::ITransaction* CreateTxScheduleConditionalErase(TEvDataShard::TEvConditionalEraseRowsResponse::TPtr& ev);

    struct TTxSyncTenant;
    NTabletFlatExecutor::ITransaction* CreateTxSyncTenant(TPathId tabletId);
    struct TTxUpdateTenant;
    NTabletFlatExecutor::ITransaction* CreateTxUpdateTenant(TEvSchemeShard::TEvUpdateTenantSchemeShard::TPtr& ev);

    struct TTxPublishToSchemeBoard;
    NTabletFlatExecutor::ITransaction* CreateTxPublishToSchemeBoard(THashMap<TTxId, TDeque<TPathId>>&& paths);
    struct TTxAckPublishToSchemeBoard;
    NTabletFlatExecutor::ITransaction* CreateTxAckPublishToSchemeBoard(TSchemeBoardEvents::TEvUpdateAck::TPtr& ev);

    struct TTxOperationPropose;
    NTabletFlatExecutor::ITransaction* CreateTxOperationPropose(TEvSchemeShard::TEvModifySchemeTransaction::TPtr& ev);

    struct TTxOperationProposeCancelTx;
    NTabletFlatExecutor::ITransaction* CreateTxOperationPropose(TEvSchemeShard::TEvCancelTx::TPtr& ev);

    struct TTxOperationProgress;
    NTabletFlatExecutor::ITransaction* CreateTxOperationProgress(TOperationId opId);

    struct TTxOperationPlanStep;
    NTabletFlatExecutor::ITransaction* CreateTxOperationPlanStep(TEvTxProcessing::TEvPlanStep::TPtr& ev);

    struct TTxUpgradeAccessDatabaseRights;
    NTabletFlatExecutor::ITransaction* CreateTxUpgradeAccessDatabaseRights(const TActorId& answerTo, bool isDryRun, std::function< NActors::IEventBase* (const TMap<TPathId, TSet<TString>>&) >);

    struct TTxMakeAccessDatabaseNoInheritable;
    NTabletFlatExecutor::ITransaction* CreateTxMakeAccessDatabaseNoInheritable(const TActorId& answerTo, bool isDryRun, std::function< NActors::IEventBase* (const TMap<TPathId, TSet<TString>>&) >);

    struct TTxServerlessStorageBilling;
    NTabletFlatExecutor::ITransaction* CreateTxServerlessStorageBilling();

    struct TTxLogin;
    NTabletFlatExecutor::ITransaction* CreateTxLogin(TEvSchemeShard::TEvLogin::TPtr &ev);


    template<class T> struct TTxOperationReply;
#define DeclareCreateTxOperationReply(TEvType, TxType) \
        NTabletFlatExecutor::ITransaction* CreateTxOperationReply(TOperationId id, TEvType::TPtr& ev);
    SCHEMESHARD_INCOMING_EVENTS(DeclareCreateTxOperationReply)
#undef DeclareCreateTxOperationReply

    void PublishToSchemeBoard(THashMap<TTxId, TDeque<TPathId>>&& paths, const TActorContext& ctx);
    void PublishToSchemeBoard(TTxId txId, TDeque<TPathId>&& paths, const TActorContext& ctx);

    void ApplyPartitionConfigStoragePatch(
        NKikimrSchemeOp::TPartitionConfig& config,
        const NKikimrSchemeOp::TPartitionConfig& patch) const;
    void FillTableDescriptionForShardIdx(
        TPathId tableId, TShardIdx shardIdx, NKikimrSchemeOp::TTableDescription* tableDescr,
        TString rangeBegin, TString rangeEnd,
        bool rangeBeginInclusive, bool rangeEndInclusive,
        bool newTable = false);
    void FillTableDescription(TPathId tableId, ui32 partitionIdx, ui64 schemaVersion, NKikimrSchemeOp::TTableDescription* tableDescr);
    static bool FillUniformPartitioning(TVector<TString>& rangeEnds, ui32 keySize, NScheme::TTypeId firstKeyColType,
                                        ui32 partitionCount, const NScheme::TTypeRegistry* typeRegistry, TString& errStr);
    static bool FillSplitPartitioning(TVector<TString>& rangeEnds, const TConstArrayRef<NScheme::TTypeId>& keyColTypes,
                                      const ::google::protobuf::RepeatedPtrField<NKikimrSchemeOp::TSplitBoundary>& boundaries,
                                      TString& errStr);

    TString FillAlterTableTxBody(TPathId tableId, TShardIdx shardIdx, TMessageSeqNo seqNo) const;
    TString FillBackupTxBody(TPathId pathId, const NKikimrSchemeOp::TBackupTask& task, ui32 shardNum, TMessageSeqNo seqNo) const;

    static void FillSeqNo(NKikimrTxDataShard::TFlatSchemeTransaction &tx, TMessageSeqNo seqNo);
    static void FillSeqNo(NKikimrTxColumnShard::TSchemaTxBody &tx, TMessageSeqNo seqNo);

    void FillAsyncIndexInfo(const TPathId& tableId, NKikimrTxDataShard::TFlatSchemeTransaction& tx);

    void DescribeTable(const TTableInfo::TPtr tableInfo, const NScheme::TTypeRegistry* typeRegistry,
                       bool fillConfig, bool fillBoundaries, NKikimrSchemeOp::TTableDescription* entry) const;
    void DescribeTableIndex(const TPathId& pathId, const TString& name, NKikimrSchemeOp::TIndexDescription& entry);
    void DescribeTableIndex(const TPathId& pathId, const TString& name, TTableIndexInfo::TPtr indexInfo, NKikimrSchemeOp::TIndexDescription& entry);
    void DescribeCdcStream(const TPathId& pathId, const TString& name, NKikimrSchemeOp::TCdcStreamDescription& desc);
    void DescribeCdcStream(const TPathId& pathId, const TString& name, TCdcStreamInfo::TPtr info, NKikimrSchemeOp::TCdcStreamDescription& desc);
    void DescribeSequence(const TPathId& pathId, const TString& name, NKikimrSchemeOp::TSequenceDescription& desc);
    void DescribeSequence(const TPathId& pathId, const TString& name, TSequenceInfo::TPtr info, NKikimrSchemeOp::TSequenceDescription& desc);
    void DescribeReplication(const TPathId& pathId, const TString& name, NKikimrSchemeOp::TReplicationDescription& desc);
    void DescribeReplication(const TPathId& pathId, const TString& name, TReplicationInfo::TPtr info, NKikimrSchemeOp::TReplicationDescription& desc);
    static void FillTableBoundaries(const TTableInfo::TPtr tableInfo, google::protobuf::RepeatedPtrField<NKikimrSchemeOp::TSplitBoundary>& boundaries);

    void Handle(TEvSchemeShard::TEvInitRootShard::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvSchemeShard::TEvInitTenantSchemeShard::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvSchemeShard::TEvModifySchemeTransaction::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvSchemeShard::TEvDescribeScheme::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvSchemeShard::TEvNotifyTxCompletion::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvSchemeShard::TEvCancelTx::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvPrivate::TEvProgressOperation::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvTabletPipe::TEvClientConnected::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvTabletPipe::TEvClientDestroyed::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvTabletPipe::TEvServerConnected::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvTabletPipe::TEvServerDisconnected::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvHive::TEvCreateTabletReply::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvHive::TEvAdoptTabletReply::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvHive::TEvDeleteTabletReply::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvPrivate::TEvSubscribeToShardDeletion::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvHive::TEvDeleteOwnerTabletsReply::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvPersQueue::TEvDropTabletReply::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvColumnShard::TEvProposeTransactionResult::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvColumnShard::TEvNotifyTxCompletionResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NSequenceShard::TEvSequenceShard::TEvCreateSequenceResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NSequenceShard::TEvSequenceShard::TEvDropSequenceResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NSequenceShard::TEvSequenceShard::TEvUpdateSequenceResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NSequenceShard::TEvSequenceShard::TEvFreezeSequenceResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NSequenceShard::TEvSequenceShard::TEvRestoreSequenceResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NSequenceShard::TEvSequenceShard::TEvRedirectSequenceResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NReplication::TEvController::TEvCreateReplicationResult::TPtr &ev, const TActorContext &ctx);
    void Handle(NReplication::TEvController::TEvDropReplicationResult::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvDataShard::TEvProposeTransactionResult::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvDataShard::TEvSchemaChanged::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvDataShard::TEvStateChanged::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvPersQueue::TEvUpdateConfigResponse::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvSubDomain::TEvConfigureStatus::TPtr &ev, const TActorContext &ctx);
    void Handle(TEvBlockStore::TEvUpdateVolumeConfigResponse::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvFileStore::TEvUpdateConfigResponse::TPtr& ev, const TActorContext& ctx);
    void Handle(NKesus::TEvKesus::TEvSetConfigResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvInitTenantSchemeShardResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvPublishTenantAsReadOnly::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvPublishTenantAsReadOnlyResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvPublishTenant::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvPublishTenantResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvMigrateSchemeShard::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvMigrateSchemeShardResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvDataShard::TEvMigrateSchemeShardResponse::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvDataShard::TEvCompactTableResult::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvSchemeShard::TEvSyncTenantSchemeShard::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvUpdateTenantSchemeShard::TPtr& ev, const TActorContext& ctx);

    void Handle(TSchemeBoardEvents::TEvUpdateAck::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvTxProcessing::TEvPlanStep::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvents::TEvPoisonPill::TPtr& ev, const TActorContext& ctx);
    void Handle(NMon::TEvRemoteHttpInfo::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvDataShard::TEvInitSplitMergeDestinationAck::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvDataShard::TEvSplitAck::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvDataShard::TEvSplitPartitioningChangedAck::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvDataShard::TEvPeriodicTableStats::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvDataShard::TEvGetTableStatsResult::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvSchemeShard::TEvFindTabletSubDomainPathId::TPtr& ev, const TActorContext& ctx);

    void ScheduleConditionalEraseRun(const TActorContext& ctx);
    void Handle(TEvPrivate::TEvRunConditionalErase::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvDataShard::TEvConditionalEraseRowsResponse::TPtr& ev, const TActorContext& ctx);

    void Handle(NSysView::TEvSysView::TEvGetPartitionStats::TPtr& ev, const TActorContext& ctx);

    void ScheduleServerlessStorageBilling(const TActorContext& ctx);
    void Handle(TEvPrivate::TEvServerlessStorageBilling::TPtr& ev, const TActorContext& ctx);

    void Handle(NConsole::TEvConfigsDispatcher::TEvSetConfigSubscriptionResponse::TPtr &ev, const TActorContext &ctx);
    void Handle(NConsole::TEvConsole::TEvConfigNotificationRequest::TPtr &ev, const TActorContext &ctx);

    void Handle(TEvSchemeShard::TEvLogin::TPtr& ev, const TActorContext& ctx);

    void RestartPipeTx(TTabletId tabletId, const TActorContext& ctx);

    TOperationId RouteIncomming(TTabletId tabletId, const TActorContext& ctx);

    // namespace NLongRunningCommon {
    struct TXxport {
        class TTxBase;
        template <typename TInfo, typename TEvRequest, typename TEvResponse> struct TTxGet;
        template <typename TInfo, typename TEvRequest, typename TEvResponse, typename TDerived> struct TTxList;
    };

    void Handle(TEvTxAllocatorClient::TEvAllocateResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvModifySchemeTransactionResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvIndexBuilder::TEvCreateResponse::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvNotifyTxCompletionRegistered::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvNotifyTxCompletionResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvSchemeShard::TEvCancelTxResult::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvIndexBuilder::TEvCancelResponse::TPtr& ev, const TActorContext& ctx);
    // } // NLongRunningCommon

    // namespace NExport {
    THashMap<ui64, TExportInfo::TPtr> Exports;
    THashMap<TString, TExportInfo::TPtr> ExportsByUid;
    THashMap<TTxId, std::pair<ui64, ui32>> TxIdToExport;

    void FromXxportInfo(NKikimrExport::TExport& exprt, const TExportInfo::TPtr exportInfo);

    static void PersistCreateExport(NIceDb::TNiceDb& db, const TExportInfo::TPtr exportInfo);
    static void PersistRemoveExport(NIceDb::TNiceDb& db, const TExportInfo::TPtr exportInfo);
    static void PersistExportPathId(NIceDb::TNiceDb& db, const TExportInfo::TPtr exportInfo);
    static void PersistExportState(NIceDb::TNiceDb& db, const TExportInfo::TPtr exportInfo);
    static void PersistExportItemState(NIceDb::TNiceDb& db, const TExportInfo::TPtr exportInfo, ui32 targetIdx);

    struct TExport {
        struct TTxCreate;
        struct TTxGet;
        struct TTxCancel;
        struct TTxCancelAck;
        struct TTxForget;
        struct TTxList;

        struct TTxProgress;
    };

    NTabletFlatExecutor::ITransaction* CreateTxCreateExport(TEvExport::TEvCreateExportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxGetExport(TEvExport::TEvGetExportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxCancelExport(TEvExport::TEvCancelExportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxCancelExportAck(TEvSchemeShard::TEvCancelTxResult::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxForgetExport(TEvExport::TEvForgetExportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxListExports(TEvExport::TEvListExportsRequest::TPtr& ev);

    NTabletFlatExecutor::ITransaction* CreateTxProgressExport(ui64 id);
    NTabletFlatExecutor::ITransaction* CreateTxProgressExport(TEvTxAllocatorClient::TEvAllocateResult::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgressExport(TEvSchemeShard::TEvModifySchemeTransactionResult::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgressExport(TEvSchemeShard::TEvNotifyTxCompletionResult::TPtr& ev);

    void Handle(TEvExport::TEvCreateExportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvExport::TEvGetExportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvExport::TEvCancelExportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvExport::TEvForgetExportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvExport::TEvListExportsRequest::TPtr& ev, const TActorContext& ctx);

    void ResumeExports(const TVector<ui64>& exportIds, const TActorContext& ctx);
    // } // NExport

    // namespace NImport {
    THashMap<ui64, TImportInfo::TPtr> Imports;
    THashMap<TString, TImportInfo::TPtr> ImportsByUid;
    THashMap<TTxId, std::pair<ui64, ui32>> TxIdToImport;

    void FromXxportInfo(NKikimrImport::TImport& exprt, const TImportInfo::TPtr importInfo);

    static void PersistCreateImport(NIceDb::TNiceDb& db, const TImportInfo::TPtr importInfo);
    static void PersistRemoveImport(NIceDb::TNiceDb& db, const TImportInfo::TPtr importInfo);
    static void PersistImportState(NIceDb::TNiceDb& db, const TImportInfo::TPtr importInfo);
    static void PersistImportItemState(NIceDb::TNiceDb& db, const TImportInfo::TPtr importInfo, ui32 itemIdx);
    static void PersistImportItemScheme(NIceDb::TNiceDb& db, const TImportInfo::TPtr importInfo, ui32 itemIdx);
    static void PersistImportItemDstPathId(NIceDb::TNiceDb& db, const TImportInfo::TPtr importInfo, ui32 itemIdx);

    struct TImport {
        struct TTxCreate;
        struct TTxGet;
        struct TTxCancel;
        struct TTxCancelAck;
        struct TTxForget;
        struct TTxList;

        struct TTxProgress;
    };

    NTabletFlatExecutor::ITransaction* CreateTxCreateImport(TEvImport::TEvCreateImportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxGetImport(TEvImport::TEvGetImportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxCancelImport(TEvImport::TEvCancelImportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxCancelImportAck(TEvSchemeShard::TEvCancelTxResult::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxCancelImportAck(TEvIndexBuilder::TEvCancelResponse::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxForgetImport(TEvImport::TEvForgetImportRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxListImports(TEvImport::TEvListImportsRequest::TPtr& ev);

    NTabletFlatExecutor::ITransaction* CreateTxProgressImport(ui64 id);
    NTabletFlatExecutor::ITransaction* CreateTxProgressImport(TEvPrivate::TEvImportSchemeReady::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgressImport(TEvTxAllocatorClient::TEvAllocateResult::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgressImport(TEvSchemeShard::TEvModifySchemeTransactionResult::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgressImport(TEvIndexBuilder::TEvCreateResponse::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgressImport(TEvSchemeShard::TEvNotifyTxCompletionResult::TPtr& ev);

    void Handle(TEvImport::TEvCreateImportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvImport::TEvGetImportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvImport::TEvCancelImportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvImport::TEvForgetImportRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvImport::TEvListImportsRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvPrivate::TEvImportSchemeReady::TPtr& ev, const TActorContext& ctx);

    void ResumeImports(const TVector<ui64>& ids, const TActorContext& ctx);
    // } // NImport

    void FillTableSchemaVersion(ui64 schemaVersion, NKikimrSchemeOp::TTableDescription *tableDescr) const;

    // namespace NIndexBuilder {
    TControlWrapper AllowDataColumnForIndexTable;
    TControlWrapper EnableAsyncIndexes;

    THashMap<TIndexBuildId, TIndexBuildInfo::TPtr> IndexBuilds;
    THashMap<TString, TIndexBuildInfo::TPtr> IndexBuildsByUid;
    THashMap<TTxId, TIndexBuildId> TxIdToIndexBuilds;

    // do not share pipes with operations
    // alse do not share pipes between IndexBuilds
    struct TDedicatedPipePool {
        using TMessage = std::pair<ui32, TIntrusivePtr<TEventSerializedData>>;
        using TOwnerRec = std::pair<TIndexBuildId, TTabletId>;
        NTabletPipe::TClientConfig PipeCfg;

        TMap<TIndexBuildId, TMap<TTabletId, TActorId>> Pipes;
        TMap<TActorId, TOwnerRec> Owners;

        TDedicatedPipePool();

        void Create(TIndexBuildId ownerTxId, TTabletId dst, THolder<IEventBase> message, const TActorContext& ctx);
        void Close(TIndexBuildId ownerTxId, TTabletId dst, const TActorContext& ctx);
        ui64 CloseAll(TIndexBuildId ownerTxId, const TActorContext& ctx);

        bool Has(TActorId actorId) const;
        TTabletId GetTabletId(TActorId actorId) const;
        TIndexBuildId GetOwnerId(TActorId actorId) const;
    };
    TDedicatedPipePool IndexBuildPipes;

    void PersistCreateBuildIndex(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexState(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexIssue(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexCancelRequest(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    void PersistBuildIndexInitiateTxId(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexInitiateTxStatus(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexInitiateTxDone(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    void PersistBuildIndexLockTxId(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexLockTxStatus(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexLockTxDone(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    void PersistBuildIndexApplyTxId(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexApplyTxStatus(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexApplyTxDone(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    void PersistBuildIndexUnlockTxId(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexUnlockTxStatus(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);
    void PersistBuildIndexUnlockTxDone(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    void PersistBuildIndexUploadProgress(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo, const TShardIdx& shardIdx);
    void PersistBuildIndexUploadInitiate(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo, const TShardIdx& shardIdx);
    void PersistBuildIndexBilling(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    void PersistBuildIndexForget(NIceDb::TNiceDb& db, const TIndexBuildInfo::TPtr indexInfo);

    struct TIndexBuilder {
        class TTxBase;

        class TTxCreate;
        struct TTxGet;
        struct TTxCancel;
        struct TTxForget;
        struct TTxList;

        struct TTxProgress;
        struct TTxReply;

        struct TTxPipeReset;
        struct TTxBilling;
    };

    NTabletFlatExecutor::ITransaction* CreateTxCreate(TEvIndexBuilder::TEvCreateRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxGet(TEvIndexBuilder::TEvGetRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxCancel(TEvIndexBuilder::TEvCancelRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxForget(TEvIndexBuilder::TEvForgetRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxList(TEvIndexBuilder::TEvListRequest::TPtr& ev);
    NTabletFlatExecutor::ITransaction* CreateTxProgress(TIndexBuildId id);
    NTabletFlatExecutor::ITransaction* CreateTxReply(TEvTxAllocatorClient::TEvAllocateResult::TPtr& allocateResult);
    NTabletFlatExecutor::ITransaction* CreateTxReply(TEvSchemeShard::TEvModifySchemeTransactionResult::TPtr& modifyResult);
    NTabletFlatExecutor::ITransaction* CreateTxReply(TEvSchemeShard::TEvNotifyTxCompletionResult::TPtr& modifyResult);
    NTabletFlatExecutor::ITransaction* CreateTxReply(TEvDataShard::TEvBuildIndexProgressResponse::TPtr& progress);
    NTabletFlatExecutor::ITransaction* CreatePipeRetry(TIndexBuildId indexBuildId, TTabletId tabletId);
    NTabletFlatExecutor::ITransaction* CreateTxBilling(TEvPrivate::TEvIndexBuildingMakeABill::TPtr& ev);


    void Handle(TEvIndexBuilder::TEvCreateRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvIndexBuilder::TEvGetRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvIndexBuilder::TEvCancelRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvIndexBuilder::TEvForgetRequest::TPtr& ev, const TActorContext& ctx);
    void Handle(TEvIndexBuilder::TEvListRequest::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvDataShard::TEvBuildIndexProgressResponse::TPtr& ev, const TActorContext& ctx);

    void Handle(TEvPrivate::TEvIndexBuildingMakeABill::TPtr& ev, const TActorContext& ctx);

    void Resume(const TDeque<TIndexBuildId>& indexIds, const TActorContext& ctx);
    void SetupRouting(const TDeque<TIndexBuildId>& indexIds, const TActorContext& ctx);

    // } //NIndexBuilder

public:
    void ChangeStreamShardsCount(i64 delta) override;
    void ChangeStreamShardsQuota(i64 delta) override;
    void ChangeStreamReservedStorageCount(i64 delta) override;
    void ChangeStreamReservedStorageQuota(i64 delta) override;
    void ChangeDiskSpaceTablesDataBytes(i64 delta) override;
    void ChangeDiskSpaceTablesIndexBytes(i64 delta) override;
    void ChangeDiskSpaceTablesTotalBytes(i64 delta) override;
    void ChangeDiskSpaceQuotaExceeded(i64 delta) override;
    void ChangeDiskSpaceHardQuotaBytes(i64 delta) override;
    void ChangeDiskSpaceSoftQuotaBytes(i64 delta) override;

    NLogin::TLoginProvider LoginProvider;

private:
    void OnDetach(const TActorContext &ctx) override;
    void OnTabletDead(TEvTablet::TEvTabletDead::TPtr &ev, const TActorContext &ctx) override;
    void OnActivateExecutor(const TActorContext &ctx) override;
    bool OnRenderAppHtmlPage(NMon::TEvRemoteHttpInfo::TPtr ev, const TActorContext &ctx) override;
    void DefaultSignalTabletActive(const TActorContext &ctx) override;
    void Cleanup(const TActorContext &ctx);
    void Enqueue(STFUNC_SIG) override;
    void Die(const TActorContext &ctx) override;

    bool ReassignChannelsEnabled() const override {
        return true;
    }

    const TDomainsInfo::TDomain& GetDomainDescription(const TActorContext &ctx) const;
    NKikimrSubDomains::TProcessingParams CreateRootProcessingParams(const TActorContext &ctx);
    static NTabletPipe::TClientConfig GetPipeClientConfig();

public:
    static const NKikimrConfig::TDomainsConfig& GetDomainsConfig();

    static constexpr NKikimrServices::TActivity::EType ActorActivityType() {
        return NKikimrServices::TActivity::FLAT_SCHEMESHARD_ACTOR;
    }

    TSchemeShard(const TActorId &tablet, TTabletStorageInfo *info);

    //TTabletId TabletID() const { return TTabletId(ITablet::TabletID()); }
    TTabletId SelfTabletId() const { return TTabletId(ITablet::TabletID()); }

    STFUNC(StateInit);
    STFUNC(StateConfigure);
    STFUNC(StateWork);
    STFUNC(BrokenState);

    // A helper that enforces write-only access to the internal DB (reads must be done from the
    // internal structures)
    class TRwTxBase : public NTabletFlatExecutor::TTransactionBase<TSchemeShard> {
    protected:
        TDuration ExecuteDuration;

    protected:
        TRwTxBase(TSchemeShard* self) : TBase(self) {}

    public:
        virtual ~TRwTxBase() {}

        bool Execute(NTabletFlatExecutor::TTransactionContext &txc, const TActorContext &ctx) override;
        void Complete(const TActorContext &ctx) override;

        virtual void DoExecute(NTabletFlatExecutor::TTransactionContext &txc, const TActorContext &ctx) = 0;
        virtual void DoComplete(const TActorContext &ctx) = 0;
    };
};

}
}
