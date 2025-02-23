
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(core-sys_view-processor)
target_compile_options(core-sys_view-processor PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(core-sys_view-processor PUBLIC
  contrib-libs-cxxsupp
  yutil
  ydb-core-base
  core-engine-minikql
  core-grpc_services-counters
  core-kqp-counters
  ydb-core-protos
  ydb-core-tablet
  ydb-core-tablet_flat
  core-tx-scheme_cache
)
target_sources(core-sys_view-processor PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/processor.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/processor_impl.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/schema.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/db_counters.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_init.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_init_schema.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_configure.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_collect.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_aggregate.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_interval_summary.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/sys_view/processor/tx_interval_metrics.cpp
)
