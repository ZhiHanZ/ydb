#include "cfg.h"
#include "error.h"
#include "proxy_actor.h"

#include <ydb/core/protos/sqs.pb.h>
#include <ydb/core/ymq/base/counters.h>
#include <ydb/core/ymq/base/security.h>
#include <library/cpp/actors/core/hfunc.h>

#include <util/string/builder.h>
#include <util/system/defaults.h>


namespace NKikimr::NSQS {

void TProxyActor::Bootstrap() {
    this->Become(&TProxyActor::StateFunc);

    StartTs_ = TActivationContext::Now();
    RLOG_SQS_DEBUG("Request proxy started");
    const auto& cfg = Cfg();

    if (cfg.GetYandexCloudMode()) {
        TString securityToken;
#define SQS_REQUEST_CASE(action)                                                                                              \
        const auto& request = Request_.Y_CAT(Get, action)();                                                                  \
        securityToken = ExtractSecurityToken<typename std::remove_reference<decltype(request)>::type, TCredentials>(request);
        SQS_SWITCH_REQUEST(Request_, Y_VERIFY(false));
#undef SQS_REQUEST_CASE
        TStringBuf tokenBuf(securityToken);
        UserName_ = TString(tokenBuf.NextTok(':'));
        FolderId_ = TString(tokenBuf.NextTok(':'));

        // TODO: handle empty cloud id better
        RLOG_SQS_DEBUG("Proxy actor: used " << UserName_ << " as an account name and " << QueueName_ << " as a queue name");
    }

    if (!UserName_ || !QueueName_) {
        RLOG_SQS_WARN("Validation error: No " << (!UserName_ ? "user name" : "queue name") << " in proxy actor");
        SendErrorAndDie(NErrors::INVALID_PARAMETER_VALUE, "Both account and queue name should be specified.");
        return;
    }

    if (cfg.GetRequestTimeoutMs()) {
        this->Schedule(TDuration::MilliSeconds(cfg.GetRequestTimeoutMs()), new TEvWakeup(), TimeoutCookie_.Get());
    }

    RequestConfiguration();
}

void TProxyActor::HandleConfiguration(TSqsEvents::TEvConfiguration::TPtr& ev) {
    const TDuration confDuration = TActivationContext::Now() - StartTs_;
    RLOG_SQS_DEBUG("Get configuration duration: " << confDuration.MilliSeconds() << "ms");

    QueueCounters_ = std::move(ev->Get()->QueueCounters);
    UserCounters_ = std::move(ev->Get()->UserCounters);
    if (QueueCounters_) {
        auto* detailedCounters = QueueCounters_ ? QueueCounters_->GetDetailedCounters() : nullptr;
        COLLECT_HISTOGRAM_COUNTER(detailedCounters, GetConfiguration_Duration, confDuration.MilliSeconds());
    }

    if (ev->Get()->Fail) {
        RLOG_SQS_ERROR("Failed to get configuration");
        SendErrorAndDie(NErrors::INTERNAL_FAILURE, "Failed to get configuration.");
        return;
    }

    if (!ev->Get()->QueueExists) {
        SendErrorAndDie(NErrors::NON_EXISTENT_QUEUE);
        return;
    }

    Send(MakeSqsProxyServiceID(SelfId().NodeId()), MakeHolder<TSqsEvents::TEvProxySqsRequest>(Request_, UserName_, QueueName_));
}

STATEFN(TProxyActor::StateFunc) {
    switch (ev->GetTypeRewrite()) {
        hFunc(TSqsEvents::TEvConfiguration, HandleConfiguration);
        hFunc(TSqsEvents::TEvProxySqsResponse, HandleResponse);
        hFunc(TEvWakeup, HandleWakeup);
    }
}

void TProxyActor::RequestConfiguration() {
    Send(MakeSqsServiceID(SelfId().NodeId()),
        MakeHolder<TSqsEvents::TEvGetConfiguration>(
            RequestId_,
            UserName_,
            QueueName_)
    );
}

void TProxyActor::SendReplyAndDie(const NKikimrClient::TSqsResponse& resp) {
    if (ErrorResponse_) {
        RLOG_SQS_WARN("Sending error reply from proxy actor: " << resp);
    } else {
        RLOG_SQS_DEBUG("Sending reply from proxy actor: " << resp);
    }
    Cb_->DoSendReply(resp);
    PassAway();
}

void TProxyActor::SendErrorAndDie(const TErrorClass& error, const TString& message) {
    ErrorResponse_ = true;
    auto* detailedCounters = UserCounters_ ? UserCounters_->GetDetailedCounters() : nullptr;
    if (detailedCounters) {
        detailedCounters->APIStatuses.AddError(error.ErrorCode);
    }
    NKikimrClient::TSqsResponse response;
#define SQS_REQUEST_CASE(action)                                    \
    MakeError(response.Y_CAT(Mutable, action)(), error, message);   \
    response.Y_CAT(Mutable, action)()->SetRequestId(RequestId_);

    SQS_SWITCH_REQUEST(Request_, Y_VERIFY(false));

#undef SQS_REQUEST_CASE

    if (Cfg().GetYandexCloudMode()) {
        response.SetFolderId(FolderId_);
        response.SetIsFifo(false);
        response.SetResourceId(QueueName_);
    }

    SendReplyAndDie(response);
}

void TProxyActor::HandleResponse(TSqsEvents::TEvProxySqsResponse::TPtr& ev) {
    RLOG_SQS_TRACE("HandleResponse: " << ev->Get()->Record << ", status: " << ev->Get()->ProxyStatus);
    if (ev->Get()->ProxyStatus == TSqsEvents::TEvProxySqsResponse::EProxyStatus::OK) {
        SendReplyAndDie(ev->Get()->Record);
    } else {
        SendErrorAndDie(GetErrorClass(ev->Get()->ProxyStatus));
    }
}

void TProxyActor::HandleWakeup(TEvWakeup::TPtr&) {
    TString actionName;

#define SQS_REQUEST_CASE(action) actionName = Y_STRINGIZE(action);

    SQS_SWITCH_REQUEST(Request_, break;);

#undef SQS_REQUEST_CASE

    RLOG_SQS_ERROR("Proxy request timeout. User [" << UserName_ << "] Queue [" << QueueName_ << "] Action [" << actionName << "]");

    if (QueueCounters_) {
        INC_COUNTER_COUPLE(QueueCounters_, RequestTimeouts, request_timeouts_count_per_second);
    } else {
        auto rootCounters = TIntrusivePtrCntrCouple{
            GetSqsServiceCounters(AppData()->Counters, "core"),
            GetYmqPublicCounters(AppData()->Counters)
        };
        auto [userCountersCouple, queueCountersCouple] = GetUserAndQueueCounters(rootCounters, TQueuePath(Cfg().GetRoot(), UserName_, QueueName_));
        if (queueCountersCouple.SqsCounters) {
            queueCountersCouple.SqsCounters->GetCounter("RequestTimeouts", true)->Inc();
        }
//        if (queueCountersCouple.YmqCounters) {
//            queueCountersCouple.YmqCounters->GetCounter("request_timeouts_count_per_second", true)->Inc();
//        }
    }

    SendErrorAndDie(NErrors::TIMEOUT);
}

const TErrorClass& TProxyActor::GetErrorClass(TSqsEvents::TEvProxySqsResponse::EProxyStatus proxyStatus) {
    using EProxyStatus = TSqsEvents::TEvProxySqsResponse::EProxyStatus;
    switch (proxyStatus) {
    case EProxyStatus::LeaderResolvingError:
        return NErrors::LEADER_RESOLVING_ERROR;
    case EProxyStatus::SessionError:
        return NErrors::LEADER_SESSION_ERROR;
    case EProxyStatus::QueueDoesNotExist:
    case EProxyStatus::UserDoesNotExist:
        return NErrors::NON_EXISTENT_QUEUE;
    default:
        return NErrors::INTERNAL_FAILURE;
    }
}

bool TProxyActor::NeedCreateProxyActor(const NKikimrClient::TSqsRequest& req) {
#define SQS_REQUEST_CASE(action) return true;

    SQS_SWITCH_REQUEST(req, return false)

#undef SQS_REQUEST_CASE
}

bool TProxyActor::NeedCreateProxyActor(EAction action) {
    return IsProxyAction(action);
}

void TProxyActor::RetrieveUserAndQueueParameters() {
// User name might be changed later in bootstrap for cloud mode
#define SQS_REQUEST_CASE(action)                                        \
    UserName_ = Request_.Y_CAT(Get, action)().GetAuth().GetUserName();  \
    QueueName_ = Request_.Y_CAT(Get, action)().GetQueueName();          \

    SQS_SWITCH_REQUEST(Request_, throw TSQSException(NErrors::INVALID_ACTION) << "Incorrect request type")

    #undef SQS_REQUEST_CASE
}

} // namespace NKikimr::NSQS
