
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(cpp-client-ydb_persqueue_core)
target_link_libraries(cpp-client-ydb_persqueue_core PUBLIC
  contrib-libs-cxxsupp
  yutil
  tools-enum_parser-enum_serialization_runtime
  library-cpp-retry
  client-ydb_persqueue_core-impl
  cpp-client-ydb_proto
  cpp-client-ydb_driver
)
target_sources(cpp-client-ydb_persqueue_core PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/public/sdk/cpp/client/ydb_persqueue_core/proto_accessor.cpp
)
generate_enum_serilization(cpp-client-ydb_persqueue_core
  ${CMAKE_SOURCE_DIR}/ydb/public/sdk/cpp/client/ydb_persqueue_core/persqueue.h
  INCLUDE_HEADERS
  ydb/public/sdk/cpp/client/ydb_persqueue_core/persqueue.h
)
