
# This file was gererated by the build system used internally in the Yandex monorepo.
# Only simple modifications are allowed (adding source-files to targets, adding simple properties
# like target_include_directories). These modifications will be ported to original
# ya.make files by maintainers. Any complex modifications which can't be ported back to the
# original buildsystem will not be accepted.



add_library(utils-TableGen-GlobalISel)
target_include_directories(utils-TableGen-GlobalISel PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel
)
target_link_libraries(utils-TableGen-GlobalISel PUBLIC
  contrib-libs-cxxsupp
  contrib-libs-llvm12
)
target_sources(utils-TableGen-GlobalISel PRIVATE
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/CodeExpander.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchDag.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchDagEdge.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchDagInstr.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchDagOperands.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchDagPredicate.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchDagPredicateDependencyEdge.cpp
  ${CMAKE_SOURCE_DIR}/contrib/libs/llvm12/utils/TableGen/GlobalISel/GIMatchTree.cpp
)
