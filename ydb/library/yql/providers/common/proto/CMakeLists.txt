
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(providers-common-proto)
target_link_libraries(providers-common-proto PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-yql-protos
  contrib-libs-protobuf
)
target_proto_messages(providers-common-proto PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/providers/common/proto/gateways_config.proto
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/providers/common/proto/udf_resolver.proto
)
target_proto_addincls(providers-common-proto
  ./
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
)
target_proto_outs(providers-common-proto
  --cpp_out=${CMAKE_BINARY_DIR}/
  --cpp_styleguide_out=${CMAKE_BINARY_DIR}/
)
