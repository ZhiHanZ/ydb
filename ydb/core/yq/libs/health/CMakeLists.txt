
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(yq-libs-health)
target_compile_options(yq-libs-health PRIVATE
  -DUSE_CURRENT_UDF_ABI_VERSION
)
target_link_libraries(yq-libs-health PUBLIC
  contrib-libs-cxxsupp
  yutil
  cpp-actors-core
  ydb-core-mon
  yq-libs-shared_resources
  cpp-client-ydb_discovery
)
target_sources(yq-libs-health PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/yq/libs/health/health.cpp
)
