
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(grpc_streaming-ut-grpc)
set_property(TARGET grpc_streaming-ut-grpc PROPERTY
  PROTOC_EXTRA_OUTS .grpc.pb.cc .grpc.pb.h
)
target_link_libraries(grpc_streaming-ut-grpc PUBLIC
  contrib-libs-cxxsupp
  yutil
  contrib-libs-grpc
  contrib-libs-protobuf
)
target_proto_messages(grpc_streaming-ut-grpc PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/grpc_streaming/ut/grpc/streaming_service.proto
)
target_proto_addincls(grpc_streaming-ut-grpc
  ./
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/contrib/libs/protobuf/src
)
target_proto_outs(grpc_streaming-ut-grpc
  --cpp_out=${CMAKE_BINARY_DIR}/
  --cpp_styleguide_out=${CMAKE_BINARY_DIR}/
)
target_proto_plugin(grpc_streaming-ut-grpc
  grpc_cpp
  grpc_cpp
)
