
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(llvm12-lib-Remarks)
target_include_directories(llvm12-lib-Remarks PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks
)
target_link_libraries(llvm12-lib-Remarks PUBLIC
  contrib-libs-cxxsupp
  contrib-libs-llvm12
  lib-Bitstream-Reader
  llvm12-lib-Support
)
target_sources(llvm12-lib-Remarks PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/BitstreamRemarkParser.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/BitstreamRemarkSerializer.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/Remark.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/RemarkFormat.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/RemarkLinker.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/RemarkParser.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/RemarkSerializer.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/RemarkStreamer.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/RemarkStringTable.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/YAMLRemarkParser.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Remarks/YAMLRemarkSerializer.cpp
)
