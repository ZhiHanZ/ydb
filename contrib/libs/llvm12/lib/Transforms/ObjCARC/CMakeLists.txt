
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(lib-Transforms-ObjCARC)
target_include_directories(lib-Transforms-ObjCARC PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC
)
target_link_libraries(lib-Transforms-ObjCARC PUBLIC
  contrib-libs-cxxsupp
  contrib-libs-llvm12
  libs-llvm12-include
  llvm12-lib-Analysis
  llvm12-lib-IR
  llvm12-lib-Support
  lib-Transforms-Utils
)
target_sources(lib-Transforms-ObjCARC PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/DependencyAnalysis.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ObjCARC.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ObjCARCAPElim.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ObjCARCContract.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ObjCARCExpand.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ObjCARCOpts.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ProvenanceAnalysis.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/ProvenanceAnalysisEvaluator.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/lib/Transforms/ObjCARC/PtrState.cpp
)
