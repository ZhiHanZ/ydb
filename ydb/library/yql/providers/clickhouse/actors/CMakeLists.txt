
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(providers-clickhouse-actors)
target_compile_options(providers-clickhouse-actors PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(providers-clickhouse-actors PUBLIC
  contrib-libs-cxxsupp
  yutil
  yql-minikql-computation
  common-token_accessor-client
  yql-public-types
  dq-actors-compute
  providers-clickhouse-proto
  providers-common-http_gateway
)
target_sources(providers-clickhouse-actors PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/providers/clickhouse/actors/yql_ch_read_actor.cpp
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/providers/clickhouse/actors/yql_ch_source_factory.cpp
)
