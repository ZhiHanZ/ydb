
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(cpp-client-ydb_types)
target_link_libraries(cpp-client-ydb_types PUBLIC
  contrib-libs-cxxsupp
  yutil
  contrib-libs-protobuf
  cpp-grpc-client
  yql-public-issue
  tools-enum_parser-enum_serialization_runtime
)
generate_enum_serilization(cpp-client-ydb_types
  ${CMAKE_SOURCE_DIR}/ydb/public/sdk/cpp/client/ydb_types/s3_settings.h
  INCLUDE_HEADERS
  ydb/public/sdk/cpp/client/ydb_types/s3_settings.h
)
generate_enum_serilization(cpp-client-ydb_types
  ${CMAKE_SOURCE_DIR}/ydb/public/sdk/cpp/client/ydb_types/status_codes.h
  INCLUDE_HEADERS
  ydb/public/sdk/cpp/client/ydb_types/status_codes.h
)
