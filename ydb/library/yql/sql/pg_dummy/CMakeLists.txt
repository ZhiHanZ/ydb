
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yql-sql-pg_dummy)
target_compile_options(yql-sql-pg_dummy PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(yql-sql-pg_dummy PUBLIC
  contrib-libs-cxxsupp
  yutil
  library-yql-ast
  yql-sql-settings
  providers-common-codec
  yql-minikql-computation
  library-yql-minikql
  library-yql-core
)
target_sources(yql-sql-pg_dummy PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/library/yql/sql/pg_dummy/pg_sql_dummy.cpp
)
