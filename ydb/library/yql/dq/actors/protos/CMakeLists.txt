
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(dq-actors-protos)
target_link_libraries(dq-actors-protos PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-actors-protos
  api-protos
  core-issue-protos
  yql-dq-proto
  public-issue-protos
  yql-public-types
  contrib-libs-protobuf
)
target_proto_messages(dq-actors-protos PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/actors/protos/dq_events.proto
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/actors/protos/dq_stats.proto
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/actors/protos/dq_status_codes.proto
)
target_proto_addincls(dq-actors-protos
  ./
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
)
target_proto_outs(dq-actors-protos
  --cpp_out=${CMAKE_BINARY_DIR}/
  --cpp_styleguide_out=${CMAKE_BINARY_DIR}/
)
