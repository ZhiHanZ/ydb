
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(ydb-core-testlib)
target_compile_options(ydb-core-testlib PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(ydb-core-testlib PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-actors-core
  cpp-actors-interconnect
  cpp-grpc-client
  cpp-grpc-server
  grpc-server-actors
  cpp-regex-pcre
  cpp-testing-gmock_in_unittest
  cpp-testing-unittest
  ydb-core-base
  core-blobstorage-base
  core-blobstorage-pdisk
  ydb-core-client
  core-client-metadata
  core-client-minikql_compile
  core-client-server
  core-cms-console
  ydb-core-engine
  core-engine-minikql
  ydb-core-formats
  ydb-core-grpc_services
  ydb-core-health_check
  core-kesus-proxy
  core-kesus-tablet
  ydb-core-keyvalue
  ydb-core-kqp
  ydb-core-metering
  ydb-core-mind
  core-mind-address_classification
  core-mind-bscontroller
  core-mind-hive
  ydb-core-node_whiteboard
  ydb-core-protos
  ydb-core-security
  core-sys_view-processor
  core-sys_view-service
  core-testlib-actors
  core-testlib-basics
  core-tx-columnshard
  core-tx-coordinator
  core-tx-long_tx_service
  core-tx-mediator
  tx-replication-controller
  core-tx-sequenceproxy
  core-tx-sequenceshard
  core-tx-time_cast
  libs-audit-mock
  yq-libs-init
  yq-libs-mock
  yq-libs-shared_resources
  ydb-library-aclib
  library-folder_service-mock
  library-mkql_proto-protos
  library-persqueue-topic_parser
  ydb-library-security
  yql-minikql-comp_nodes
  udf-service-exception_policy
  public-lib-base
  lib-deprecated-kicli
  cpp-client-ydb_driver
  client-ydb_persqueue_public-codecs
  cpp-client-ydb_table
  ydb-services-auth
  ydb-services-cms
  ydb-services-datastreams
  ydb-services-discovery
  ydb-services-kesus
  ydb-services-persqueue_cluster_discovery
  ydb-services-persqueue_v1
  ydb-services-rate_limiter
  ydb-services-ydb
  ydb-services-yq
  ydb-core-http_proxy
)
target_sources(ydb-core-testlib PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/testlib/actor_helpers.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/testlib/fake_coordinator.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/testlib/tablet_flat_dummy.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/testlib/tablet_helpers.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/testlib/tenant_runtime.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/testlib/test_client.cpp
)
