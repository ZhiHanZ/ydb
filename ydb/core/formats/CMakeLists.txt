
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(ydb-core-formats)
target_link_libraries(ydb-core-formats PUBLIC
  contrib-libs-cxxsupp
  yutil
  libs-apache-arrow
  ydb-core-scheme
)
target_sources(ydb-core-formats PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/formats/arrow_batch_builder.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/formats/arrow_helpers.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/formats/clickhouse_block.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/formats/merging_sorted_input_stream.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/formats/program.cpp
)
