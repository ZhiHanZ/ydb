#pragma once

#include <library/cpp/actors/core/actorsystem.h>

#include <ydb/public/api/grpc/ydb_discovery_v1.grpc.pb.h>

#include <library/cpp/grpc/server/grpc_server.h>

namespace NKikimr {
namespace NGRpcService {

 class TGRpcDiscoveryService
     : public NGrpc::TGrpcServiceBase<Ydb::Discovery::V1::DiscoveryService>
 {
 public:
     TGRpcDiscoveryService(NActors::TActorSystem* system,
                     TIntrusivePtr<NMonitoring::TDynamicCounters> counters,
                     NActors::TActorId id);

     void InitService(grpc::ServerCompletionQueue* cq, NGrpc::TLoggerPtr logger) override;
     void SetGlobalLimiterHandle(NGrpc::TGlobalLimiter* limiter) override;

     bool IncRequest();
     void DecRequest();

  private:
     void SetupIncomingRequests(NGrpc::TLoggerPtr logger);

     NActors::TActorSystem* ActorSystem_;
     grpc::ServerCompletionQueue* CQ_ = nullptr;

     TIntrusivePtr<NMonitoring::TDynamicCounters> Counters_;
     NActors::TActorId GRpcRequestProxyId_;
     NGrpc::TGlobalLimiter* Limiter_ = nullptr;
 };

} // namespace NGRpcService
} // namespace NKikimr
