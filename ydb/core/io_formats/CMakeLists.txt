
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(ydb-core-io_formats)
target_compile_options(ydb-core-io_formats PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(ydb-core-io_formats PUBLIC
  contrib-libs-cxxsupp
  yutil
  contrib-libs-double-conversion
  cpp-string_utils-quote
  ydb-core-formats
  ydb-core-scheme
  ydb-library-binary_json
  ydb-library-dynumber
  yql-minikql-dom
  yql-public-decimal
  yql-public-udf
  library-yql-utils
  public-lib-scheme_types
)
target_sources(ydb-core-io_formats PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/io_formats/csv_ydb_dump.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/io_formats/csv_arrow.cpp
)
