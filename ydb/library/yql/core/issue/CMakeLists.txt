
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yql-core-issue)
target_link_libraries(yql-core-issue PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-cpp-resource
  contrib-libs-protobuf
  yql-public-issue
  core-issue-protos
)
target_sources(yql-core-issue PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/core/issue/yql_issue.cpp
)

add_global_library_for(yql-core-issue.global yql-core-issue)
target_link_libraries(yql-core-issue.global PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-cpp-resource
  contrib-libs-protobuf
  yql-public-issue
  core-issue-protos
)
target_sources(yql-core-issue.global PRIVATE
  ${CMAKE_BINARY_DIR}/ydb/library/yql/core/issue/6939e1a0697a96c4e354038ff01108b2.cpp
)
resources(yql-core-issue.global
  ${CMAKE_BINARY_DIR}/ydb/library/yql/core/issue/6939e1a0697a96c4e354038ff01108b2.cpp
  INPUTS
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/core/issue/yql_issue.txt
  KEYS
  yql_issue.txt
)
