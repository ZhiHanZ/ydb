
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(ydb_persqueue_core-ut-ut_utils)
target_compile_options(ydb_persqueue_core-ut-ut_utils PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(ydb_persqueue_core-ut-ut_utils PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-grpc-server
  cpp-testing-unittest
  ydb-core-testlib
  library-persqueue-topic_parser_public
  cpp-client-ydb_driver
  cpp-client-ydb_persqueue_core
  cpp-client-ydb_persqueue_public
  cpp-client-ydb_table
)
target_sources(ydb_persqueue_core-ut-ut_utils PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/public/sdk/cpp/client/ydb_persqueue_core/ut/ut_utils/data_plane_helpers.cpp
  ${CMAKE_SOURCE_DIR}/ydb/public/sdk/cpp/client/ydb_persqueue_core/ut/ut_utils/test_server.cpp
)
