
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(mkql_proto-ut-helpers)
target_compile_options(mkql_proto-ut-helpers PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(mkql_proto-ut-helpers PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-yql-minikql
  yql-minikql-comp_nodes
  yql-minikql-computation
  yql-minikql-invoke_builtins
  cpp-testing-unittest
  contrib-libs-protobuf
)
target_sources(mkql_proto-ut-helpers PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/mkql_proto/ut/helpers/helpers.cpp
)
