
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(llvm12-lib-Target)
target_include_directories(llvm12-lib-Target PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Target
)
target_link_libraries(llvm12-lib-Target PUBLIC
  contrib-libs-cxxsupp
  contrib-libs-llvm12
  libs-llvm12-include
  llvm12-lib-Analysis
  llvm12-lib-IR
  llvm12-lib-MC
  llvm12-lib-Support
)
target_sources(llvm12-lib-Target PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Target/Target.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Target/TargetIntrinsicInfo.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Target/TargetLoweringObjectFile.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Target/TargetMachine.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Target/TargetMachineC.cpp
)
