
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(core-blobstorage-dsproxy)
target_link_libraries(core-blobstorage-dsproxy PUBLIC
  contrib-libs-cxxsupp
  yutil
  monlib-dynamic_counters-percentile
  ydb-core-base
  core-blobstorage-backpressure
  core-blobstorage-base
  core-blobstorage-groupinfo
  core-blobstorage-storagepoolmon
  blobstorage-vdisk-ingress
  ydb-core-control
  ydb-core-util
)
target_sources(core-blobstorage-dsproxy PRIVATE
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_impl.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_encrypt.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_request.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_stat.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_state.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_blackboard.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_block.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_collect.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_discover.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_discover_m3dc.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_discover_m3of4.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_get.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_get_impl.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_indexrestoreget.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_mon.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_monactor.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_multicollect.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_multiget.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_nodemon.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_nodemonactor.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_patch.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_put.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_put_impl.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_range.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_status.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/dsproxy_strategy_base.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/group_sessions.cpp
  ${CMAKE_SOURCE_DIR}/ydb/core/blobstorage/dsproxy/blobstorage_backoff.cpp
)
