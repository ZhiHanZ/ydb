
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yq-libs-control_plane_storage)
target_compile_options(yq-libs-control_plane_storage PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(yq-libs-control_plane_storage PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-cpp-lwtrace
  cpp-protobuf-interop
  ydb-core-base
  ydb-core-mon
  libs-actors-logging
  yq-libs-common
  yq-libs-config
  libs-config-protos
  libs-control_plane_storage-events
  libs-control_plane_storage-internal
  libs-control_plane_storage-proto
  yq-libs-db_schema
  libs-graph_params-proto
  yq-libs-shared_resources
  yq-libs-ydb
  ydb-library-security
  api-protos
  cpp-client-ydb_scheme
  cpp-client-ydb_table
  yql-public-issue
)
target_sources(yq-libs-control_plane_storage PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/control_plane_storage_counters.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/exceptions.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/in_memory_control_plane_storage.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/probes.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/util.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/validators.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/ydb_control_plane_storage.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/ydb_control_plane_storage_bindings.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/ydb_control_plane_storage_connections.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/control_plane_storage/ydb_control_plane_storage_queries.cpp
)
