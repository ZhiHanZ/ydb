
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(common-schema-expr)
target_link_libraries(common-schema-expr PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-cpp-yson
  cpp-yson-node
  library-yql-ast
  yql-public-issue
  yql-public-udf
  library-yql-utils
  common-schema-parser
  yql-parser-pg_catalog
)
target_sources(common-schema-expr PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/providers/common/schema/expr/yql_expr_schema.cpp
)
