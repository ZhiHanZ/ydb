
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yq-libs-protos)
target_link_libraries(yq-libs-protos PUBLIC
  contrib-libs-cxxsupp
  yutil
  api-protos
  dq-actors-protos
  contrib-libs-protobuf
)
target_proto_messages(yq-libs-protos PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/protos/yq_private.proto
)
target_proto_addincls(yq-libs-protos
  ./
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
)
target_proto_outs(yq-libs-protos
  --cpp_out=${CMAKE_BINARY_DIR}/
  --cpp_styleguide_out=${CMAKE_BINARY_DIR}/
)
