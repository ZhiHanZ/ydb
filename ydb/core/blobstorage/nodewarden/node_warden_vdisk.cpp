#include "node_warden_impl.h"

#include <ydb/core/blobstorage/crypto/default.h>

#include <util/string/split.h>

namespace NKikimr::NStorage {

    void TNodeWarden::DestroyLocalVDisk(TVDiskRecord& vdisk) {
        STLOG(PRI_INFO, BS_NODE, NW35, "DestroyLocalVDisk", (VDiskId, vdisk.GetVDiskId()), (VSlotId, vdisk.GetVSlotId()));
        Y_VERIFY(!vdisk.RuntimeData);

        const TVSlotId vslotId = vdisk.GetVSlotId();
        StopAggregator(MakeBlobStorageVDiskID(vslotId.NodeId, vslotId.PDiskId, vslotId.VDiskSlotId));

        if (vdisk.WhiteboardVDiskId) {
            Send(WhiteboardId, new NNodeWhiteboard::TEvWhiteboard::TEvVDiskStateDelete(*vdisk.WhiteboardVDiskId));
            vdisk.WhiteboardVDiskId.reset();
        }
    }

    void TNodeWarden::PoisonLocalVDisk(TVDiskRecord& vdisk) {
        STLOG(PRI_INFO, BS_NODE, NW35, "PoisonLocalVDisk", (VDiskId, vdisk.GetVDiskId()), (VSlotId, vdisk.GetVSlotId()),
            (RuntimeData, vdisk.RuntimeData.has_value()));

        if (vdisk.RuntimeData) {
            vdisk.TIntrusiveListItem<TVDiskRecord, TGroupRelationTag>::Unlink();
            TActivationContext::Send(new IEventHandle(TEvents::TSystem::Poison, 0, vdisk.GetVDiskServiceId(), {}, nullptr, 0));
            vdisk.RuntimeData.reset();
        }

        switch (vdisk.ScrubState) {
            case TVDiskRecord::EScrubState::IDLE: // no scrub in progress at all
            case TVDiskRecord::EScrubState::QUERY_START_QUANTUM: // VDisk was waiting for quantum and didn't get it
            case TVDiskRecord::EScrubState::QUANTUM_FINISHED: // quantum finished, no work is in progress
            case TVDiskRecord::EScrubState::QUANTUM_FINISHED_AND_WAITING_FOR_NEXT_ONE: // like QUERY_START_QUANTUM
                break;

            case TVDiskRecord::EScrubState::IN_PROGRESS: { // scrub is in progress, report scrub stop to BS_CONTROLLER
                const TVSlotId vslotId = vdisk.GetVSlotId();
                SendToController(std::make_unique<TEvBlobStorage::TEvControllerScrubQuantumFinished>(vslotId.NodeId,
                    vslotId.PDiskId, vslotId.VDiskSlotId));
                break;
            }
        }
        vdisk.ScrubState = TVDiskRecord::EScrubState::IDLE;
        vdisk.QuantumFinished.Clear();
        vdisk.ScrubCookie = 0; // disable reception of Scrub messages from this disk
        vdisk.ScrubCookieForController = 0; // and from controller too
        vdisk.Status = NKikimrBlobStorage::EVDiskStatus::ERROR;

        SendDiskMetrics(false);
    }

    void TNodeWarden::StartLocalVDiskActor(TVDiskRecord& vdisk, TDuration yardInitDelay) {
        const TVSlotId vslotId = vdisk.GetVSlotId();
        const ui64 pdiskGuid = vdisk.Config.GetVDiskLocation().GetPDiskGuid();
        const bool restartInFlight = InFlightRestartedPDisks.count({vslotId.NodeId, vslotId.PDiskId});
        const bool donorMode = vdisk.Config.HasDonorMode();

        STLOG(PRI_DEBUG, BS_NODE, NW23, "StartLocalVDiskActor", (RestartInFlight, restartInFlight),
            (SlayInFlight, vdisk.SlayInFlight), (VDiskId, vdisk.GetVDiskId()), (VSlotId, vslotId),
            (PDiskGuid, pdiskGuid), (DonorMode, donorMode));

        if (restartInFlight || vdisk.SlayInFlight) {
            return;
        }

        // find underlying PDisk and determine its media type
        auto pdiskIt = LocalPDisks.find({vslotId.NodeId, vslotId.PDiskId});
        Y_VERIFY_S(pdiskIt != LocalPDisks.end(), "PDiskId# " << vslotId.NodeId << ":" << vslotId.PDiskId << " not found");
        auto& pdisk = pdiskIt->second;
        Y_VERIFY_S(pdisk.Record.GetPDiskGuid() == pdiskGuid, "PDiskId# " << vslotId.NodeId << ":" << vslotId.PDiskId << " PDiskGuid mismatch");
        const TPDiskCategory::EDeviceType deviceType = TPDiskCategory(pdisk.Record.GetPDiskCategory()).Type();

        const TActorId pdiskServiceId = MakeBlobStoragePDiskID(vslotId.NodeId, vslotId.PDiskId);
        const TActorId vdiskServiceId = MakeBlobStorageVDiskID(vslotId.NodeId, vslotId.PDiskId, vslotId.VDiskSlotId);

        // generate correct VDiskId (based on relevant generation of containing group) and groupInfo pointer
        Y_VERIFY(!vdisk.RuntimeData);
        TVDiskID vdiskId = vdisk.GetVDiskId();
        TIntrusivePtr<TBlobStorageGroupInfo> groupInfo;

        if (!donorMode) {
            // find group containing VDisk being started
            const auto it = Groups.find(vdisk.GetGroupId());
            if (it == Groups.end()) {
                STLOG_DEBUG_FAIL(BS_NODE, NW09, "group not found while starting VDisk actor",
                    (GroupId, vdisk.GetGroupId()), (VDiskId, vdiskId), (Config, vdisk.Config));
                return;
            }
            auto& group = it->second;

            // ensure the group has correctly filled protobuf (in case when there is no relevant info pointer)
            if (!group.Group) {
                STLOG_DEBUG_FAIL(BS_NODE, NW13, "group configuration does not contain protobuf to start VDisk",
                    (GroupId, it->first), (VDiskId, vdiskId), (Config, vdisk.Config));
                return;
            }

            // obtain group info pointer
            if (group.Info) {
                groupInfo = group.Info;
            } else {
                TStringStream err;
                groupInfo = TBlobStorageGroupInfo::Parse(*group.Group, nullptr, nullptr);
            }

            // check that VDisk belongs to active VDisks of the group
            const ui32 orderNumber = groupInfo->GetOrderNumber(TVDiskIdShort(vdiskId));
            if (groupInfo->GetActorId(orderNumber) != vdiskServiceId) {
                return;
            }

            // generate correct VDisk id
            vdiskId = TVDiskID(groupInfo->GroupID, groupInfo->GroupGeneration, vdiskId);

            // entangle VDisk with the group
            group.VDisksOfGroup.PushBack(&vdisk);
        } else {
            const auto& dm = vdisk.Config.GetDonorMode();
            TBlobStorageGroupType type(static_cast<TBlobStorageGroupType::EErasureSpecies>(dm.GetErasureSpecies()));

            // generate desired group topology
            TBlobStorageGroupInfo::TTopology topology(type, dm.GetNumFailRealms(), dm.GetNumFailDomainsPerFailRealm(),
                dm.GetNumVDisksPerFailDomain());

            // fill in dynamic info
            TBlobStorageGroupInfo::TDynamicInfo dyn(vdiskId.GroupID, vdiskId.GroupGeneration);
            for (ui32 i = dm.GetNumFailRealms() * dm.GetNumFailDomainsPerFailRealm() * dm.GetNumVDisksPerFailDomain(); i; --i) {
                dyn.PushBackActorId(TActorId());
            }

            // create fake group info
            groupInfo = MakeIntrusive<TBlobStorageGroupInfo>(std::move(topology), std::move(dyn),
                vdisk.Config.GetStoragePoolName(), Nothing(), deviceType);
        }

        const auto kind = vdisk.Config.HasVDiskKind() ? vdisk.Config.GetVDiskKind() : NKikimrBlobStorage::TVDiskKind::Default;

        // just some random number of indicate VDisk actor restart
        const ui64 whiteboardInstanceGuid = RandomNumber<ui64>();

        const ui64 scrubCookie = ++LastScrubCookie;

        std::vector<std::pair<TVDiskID, TActorId>> donorDiskIds;
        for (const auto& donor : vdisk.Config.GetDonors()) {
            const TVSlotId donorSlot(donor.GetVDiskLocation());
            donorDiskIds.emplace_back(VDiskIDFromVDiskID(donor.GetVDiskId()), donorSlot.GetVDiskServiceId());
        }

        TVDiskConfig::TBaseInfo baseInfo(vdiskId, pdiskServiceId, pdiskGuid, vslotId.PDiskId, deviceType,
            vslotId.VDiskSlotId, kind, NextLocalPDiskInitOwnerRound(), groupInfo->GetStoragePoolName(), donorMode,
            donorDiskIds, scrubCookie, whiteboardInstanceGuid);

        baseInfo.ReplPDiskReadQuoter = pdiskIt->second.ReplPDiskReadQuoter;
        baseInfo.ReplPDiskWriteQuoter = pdiskIt->second.ReplPDiskWriteQuoter;
        baseInfo.ReplNodeRequestQuoter = ReplNodeRequestQuoter;
        baseInfo.ReplNodeResponseQuoter = ReplNodeResponseQuoter;
        baseInfo.YardInitDelay = yardInitDelay;

        TIntrusivePtr<TVDiskConfig> vdiskConfig = Cfg->AllVDiskKinds->MakeVDiskConfig(baseInfo);
        vdiskConfig->EnableVDiskCooldownTimeout = Cfg->EnableVDiskCooldownTimeout;
        vdiskConfig->ReplPausedAtStart = Cfg->VDiskReplPausedAtStart;
        vdiskConfig->EnableVPatch = EnableVPatch;

        // issue initial report to whiteboard before creating actor to avoid races
        Send(WhiteboardId, new NNodeWhiteboard::TEvWhiteboard::TEvVDiskStateUpdate(vdiskId, groupInfo->GetStoragePoolName(),
            vslotId.PDiskId, vslotId.VDiskSlotId, pdiskGuid, kind, donorMode, whiteboardInstanceGuid));
        vdisk.WhiteboardVDiskId.emplace(vdiskId);

        // create an actor
        auto *as = TActivationContext::ActorSystem();
        as->RegisterLocalService(vdiskServiceId, as->Register(CreateVDisk(vdiskConfig, groupInfo, AppData()->Counters),
            TMailboxType::Revolving, AppData()->SystemPoolId));

        STLOG(PRI_DEBUG, BS_NODE, NW24, "StartLocalVDiskActor done", (VDiskId, vdisk.GetVDiskId()), (VSlotId, vslotId),
            (PDiskGuid, pdiskGuid));

        // for dynamic groups -- start state aggregator
        if (TGroupID(groupInfo->GroupID).ConfigurationType() == GroupConfigurationTypeDynamic) {
            StartAggregator(vdiskServiceId, groupInfo->GroupID);
        }

        Y_VERIFY(vdisk.ScrubState == TVDiskRecord::EScrubState::IDLE);
        Y_VERIFY(!vdisk.QuantumFinished.ByteSizeLong());
        Y_VERIFY(!vdisk.ScrubCookie);

        vdisk.RuntimeData.emplace(TVDiskRecord::TRuntimeData{
            .GroupInfo = groupInfo,
            .OrderNumber = groupInfo->GetOrderNumber(TVDiskIdShort(vdiskId)),
            .DonorMode = donorMode
        });

        vdisk.Status = NKikimrBlobStorage::EVDiskStatus::INIT_PENDING;
        vdisk.ReportedVDiskStatus.reset();
        vdisk.ScrubCookie = scrubCookie;
    }

    void TNodeWarden::ApplyServiceSetVDisks(const NKikimrBlobStorage::TNodeWardenServiceSet& serviceSet) {
        for (const auto& vdisk : serviceSet.GetVDisks()) {
            ApplyLocalVDiskInfo(vdisk);
        }
        SendDiskMetrics(false);
    }

    void TNodeWarden::ApplyLocalVDiskInfo(const NKikimrBlobStorage::TNodeWardenServiceSet::TVDisk& vdisk) {
        // ApplyLocalVDiskInfo invocation may occur only in the following cases:
        //
        // 1. Starting and reading data from cache.
        // 2. Receiving RegisterNode response with comprehensive configuration.
        // 3. Processing GroupReconfigurationWipe command.
        // 4. Deleting VSlot during group reconfiguration or donor termination.
        // 5. Making VDisk a donor one.
        // 6. Updating VDisk generation when modifying group.
        //
        // The main idea of this command is when VDisk is created, it does not change its configuration. It may be
        // wiped out several times, it may become a donor and then it may be destroyed. That is a possible life cycle
        // of a VDisk in the occupied slot.

        if (!vdisk.HasVDiskID() || !vdisk.HasVDiskLocation()) {
            STLOG_DEBUG_FAIL(BS_NODE, NW30, "weird VDisk configuration", (Record, vdisk));
            return;
        }

        const auto& loc = vdisk.GetVDiskLocation();
        if (loc.GetNodeID() != LocalNodeId) {
            if (TGroupID(vdisk.GetVDiskID().GetGroupID()).ConfigurationType() != GroupConfigurationTypeStatic) {
                STLOG_DEBUG_FAIL(BS_NODE, NW31, "incorrect NodeId in VDisk configuration", (Record, vdisk), (NodeId, LocalNodeId));
            }
            return;
        }

        const TVSlotId vslotId(loc);
        const auto [it, inserted] = LocalVDisks.try_emplace(vslotId);
        auto& record = it->second;
        if (!inserted) {
            // -- check that configuration did not change
        }
        record.Config.CopyFrom(vdisk);

        if (vdisk.GetDoDestroy() || vdisk.GetEntityStatus() == NKikimrBlobStorage::EEntityStatus::DESTROY) {
            if (record.UnderlyingPDiskDestroyed) {
                PoisonLocalVDisk(record);
                SendVDiskReport(vslotId, record.GetVDiskId(), NKikimrBlobStorage::TEvControllerNodeReport::DESTROYED);
                record.UnderlyingPDiskDestroyed = false;
            } else {
                Slay(record);
            }
            DestroyLocalVDisk(record);
            LocalVDisks.erase(it);
        } else if (vdisk.GetDoWipe()) {
            Slay(record);
        } else if (!record.RuntimeData) {
            StartLocalVDiskActor(record, TDuration::Zero());
        } else if (record.RuntimeData->DonorMode < record.Config.HasDonorMode()) {
            PoisonLocalVDisk(record);
            StartLocalVDiskActor(record, TDuration::Seconds(15) /* PDisk confidence delay */);
        }
    }

    void TNodeWarden::Slay(TVDiskRecord& vdisk) {
        STLOG(PRI_INFO, BS_NODE, NW33, "Slay", (VDiskId, vdisk.GetVDiskId()), (VSlotId, vdisk.GetVSlotId()),
            (SlayInFlight, vdisk.SlayInFlight));
        if (!vdisk.SlayInFlight) {
            PoisonLocalVDisk(vdisk);
            const TVSlotId vslotId = vdisk.GetVSlotId();
            const TActorId pdiskServiceId = MakeBlobStoragePDiskID(vslotId.NodeId, vslotId.PDiskId);
            Send(pdiskServiceId, new NPDisk::TEvSlay(vdisk.GetVDiskId(), NextLocalPDiskInitOwnerRound(),
                vslotId.PDiskId, vslotId.VDiskSlotId));
            vdisk.SlayInFlight = true;
        }
    }

    void TNodeWarden::Handle(TEvBlobStorage::TEvDropDonor::TPtr ev) {
        auto *msg = ev->Get();
        STLOG(PRI_INFO, BS_NODE, NW34, "TEvDropDonor", (VSlotId, TVSlotId(msg->NodeId, msg->PDiskId, msg->VSlotId)),
            (VDiskId, msg->VDiskId));
        SendDropDonorQuery(msg->NodeId, msg->PDiskId, msg->VSlotId, msg->VDiskId);
    }

    void TNodeWarden::UpdateGroupInfoForDisk(TVDiskRecord& vdisk, const TIntrusivePtr<TBlobStorageGroupInfo>& newInfo) {
        if (!vdisk.RuntimeData) {
            return;
        }

        TIntrusivePtr<TBlobStorageGroupInfo>& currentInfo = vdisk.RuntimeData->GroupInfo;
        Y_VERIFY(newInfo->GroupID == currentInfo->GroupID);

        const ui32 orderNumber = vdisk.RuntimeData->OrderNumber;
        const TActorId vdiskServiceId = vdisk.GetVDiskServiceId();

        if (newInfo->GetActorId(orderNumber) != vdiskServiceId) {
            // this disk is in donor mode, we don't care about generation change; donor modes are operated by BSC solely
            return;
        }

        // update generation and send update message
        currentInfo = newInfo;
        const TVDiskID newVDiskId = currentInfo->GetVDiskId(orderNumber);
        vdisk.WhiteboardVDiskId.emplace(newVDiskId);
        Send(vdiskServiceId, new TEvVGenerationChange(newVDiskId, currentInfo));
    }

} // NKikimr::NStorage
