
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yql-dq-proto)
target_link_libraries(yql-dq-proto PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-actors-protos
  contrib-libs-protobuf
)
target_proto_messages(yql-dq-proto PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/proto/dq_checkpoint.proto
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/proto/dq_state_load_plan.proto
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/proto/dq_tasks.proto
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/dq/proto/dq_transport.proto
)
target_proto_addincls(yql-dq-proto
  ./
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
)
target_proto_outs(yql-dq-proto
  --cpp_out=${CMAKE_BINARY_DIR}/
  --cpp_styleguide_out=${CMAKE_BINARY_DIR}/
)
