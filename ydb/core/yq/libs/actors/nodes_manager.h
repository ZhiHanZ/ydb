#pragma once

#include <ydb/library/yql/providers/common/metrics/service_counters.h>
#include <ydb/core/yq/libs/events/events.h>
#include <ydb/core/yq/libs/shared_resources/shared_resources.h>

#include <ydb/library/yql/minikql/computation/mkql_computation_node.h>
#include <ydb/library/yql/providers/dq/provider/yql_dq_gateway.h>
#include <ydb/library/yql/providers/dq/worker_manager/interface/counters.h>
#include <ydb/library/yql/providers/dq/actors/proto_builder.h>
#include <ydb/library/yql/providers/common/http_gateway/yql_http_gateway.h>

#include <library/cpp/actors/core/actorsystem.h>
#include <library/cpp/time_provider/time_provider.h>
#include <library/cpp/random_provider/random_provider.h>

namespace NYq {

using namespace NActors;

TActorId MakeNodesManagerId();

IActor* CreateNodesManager(
    const NYql::NDqs::TWorkerManagerCounters& workerManagerCounters,
    TIntrusivePtr<ITimeProvider> timeProvider,
    TIntrusivePtr<IRandomProvider> randomProvider,
    const ::NYql::NCommon::TServiceCounters& serviceCounters,
    const NConfig::TPrivateApiConfig& privateApiConfig,
    const NYq::TYqSharedResources::TPtr& yqSharedResources,
    const ui32& icPort,
    const TString& tenant = "",
    ui64 mkqlInitialMemoryLimit = 0);

}
