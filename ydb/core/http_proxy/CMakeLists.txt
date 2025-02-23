
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(ydb-core-http_proxy)
target_compile_options(ydb-core-http_proxy PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(ydb-core-http_proxy PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-actors-http
  cpp-actors-core
  ydb-core-base
  ydb-core-protos
  core-grpc_services-local_rpc
  yql-public-issue
  library-http_proxy-authorization
  library-http_proxy-error
  ydb-library-naming_conventions
  cpp-client-ydb_datastreams
  cpp-client-ydb_persqueue_core
  client-ydb_persqueue_public-codecs
  ydb-services-datastreams
)
target_sources(ydb-core-http_proxy PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/http_proxy/http_req.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/http_proxy/metrics_actor.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/http_proxy/grpc_service.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/http_proxy/discovery_actor.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/http_proxy/http_service.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/http_proxy/auth_factory.cpp
)
