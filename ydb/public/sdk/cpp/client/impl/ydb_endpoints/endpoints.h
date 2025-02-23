#pragma once

#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <ydb/public/sdk/cpp/client/impl/ydb_stats/stats.h>
#include <ydb/public/sdk/cpp/client/impl/ydb_internal/common/type_switcher.h>

namespace NYdb {

struct TEndpointRecord {
    TStringType Endpoint;
    i32 Priority;
    TStringType SslTargetNameOverride;
    ui64 NodeId = 0;

    TEndpointRecord()
        : Endpoint()
        , Priority(0)
        , SslTargetNameOverride()
        , NodeId(0)
    {
    }

    TEndpointRecord(TStringType endpoint, i32 priority, TStringType sslTargetNameOverride = TStringType(), ui64 nodeId = 0)
        : Endpoint(std::move(endpoint))
        , Priority(priority)
        , SslTargetNameOverride(std::move(sslTargetNameOverride))
        , NodeId(nodeId)
    {
    }

    explicit operator bool() const {
        return !Endpoint.empty();
    }

    bool operator<(const TEndpointRecord& rhs) const {
        return Priority < rhs.Priority;
    }
};

struct TEndpointKey {
    TStringType Endpoint;
    ui64 NodeId = 0;

    TEndpointKey()
        : Endpoint()
        , NodeId(0)
    {}

    TEndpointKey(TStringType endpoint, ui64 nodeId)
        : Endpoint(std::move(endpoint))
        , NodeId(nodeId)
    {}

    const TStringType& GetEndpoint() const {
        return Endpoint;
    }

    const ui64& GetNodeId() const {
        return NodeId;
    }
};

class IObjRegistryHandle {
public:
    virtual ~IObjRegistryHandle() = default;
    virtual size_t Size() const = 0;
};

class TEndpointObj;
class TEndpointElectorSafe {
public:
    TEndpointElectorSafe() = default;

    // Sets new endpoints, returns removed
    std::vector<TStringType> SetNewState(std::vector<TEndpointRecord>&& records);

    // Allows to get stats
    void SetStatCollector(const NSdkStats::TStatCollector::TEndpointElectorStatCollector& endpointStatCollector);

    // Returns prefered (if presents) or best endpoint
    TEndpointRecord GetEndpoint(const TEndpointKey& preferredEndpoint) const;

    // Move endpoint to the end
    void PessimizeEndpoint(const TStringType& endpoint);

    // Returns % of pessimized endpoints
    int GetPessimizationRatio() const;

    // Associate object with the endpoint
    // Returns false if no required endpoint, or object already registered
    bool LinkObjToEndpoint(const TEndpointKey& endpoint, TEndpointObj* obj, const void* tag);

    // Perform some action for each object group associated with endpoint
    using THandleCb = std::function<void(ui64 nodeId, const IObjRegistryHandle& handle)>;
    void ForEachEndpoint(const THandleCb& cb, i32 minPriority, i32 maxPriority, const void* tag) const;

    class TObjRegistry;
private:
    using TTaggedObjRegistry = std::unordered_map<const void*, std::shared_ptr<TObjRegistry>>;

    struct TKnownEndpoint {
        TEndpointRecord Record;
        TTaggedObjRegistry TaggedObjs;
    };

private:
    mutable std::shared_mutex Mutex_;
    std::vector<TEndpointRecord> Records_;
    std::unordered_map<TStringType, TEndpointRecord> KnownEndpoints_;
    std::unordered_map<ui64, TKnownEndpoint> KnownEndpointsByNodeId_;
    i32 BestK_ = -1;
    std::atomic_int PessimizationRatio_ = 0;
    NSdkStats::TAtomicCounter<NMonitoring::TIntGauge> EndpointCountGauge_;
    NSdkStats::TAtomicCounter<NMonitoring::TIntGauge> PessimizationRatioGauge_;
    NSdkStats::TAtomicCounter<NMonitoring::TIntGauge> EndpointActiveGauge_;
};

// Used to track object
// The derived class must call Unlink() before destroying
class TEndpointObj {
    friend class TEndpointElectorSafe;
public:
    virtual ~TEndpointObj() = default;

    virtual void OnEndpointRemoved() {}

    size_t ObjectCount() const;
    bool ObjectRegistred() const;
    void Unlink();

private:
    bool Link(std::shared_ptr<TEndpointElectorSafe::TObjRegistry> registry);
    std::shared_ptr<TEndpointElectorSafe::TObjRegistry> ObjRegistry_;
};

} // namespace NYdb
