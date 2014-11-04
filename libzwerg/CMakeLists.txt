FLEX_TARGET (Lexer lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
BISON_TARGET (Parser parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)

# Core pseudo-library to reuse in test-parser.
ADD_LIBRARY (libzwerg_core OBJECT

  build.cc
  builtin-closure.cc
  builtin-cmp.cc
  builtin-cst.cc
  builtin-shf.cc
  builtin.cc
  constant.cc
  init.cc
  int.cc
  lexer.cc
  op.cc
  overload.cc
  parser.cc
  selector.cc
  stack.cc
  tree.cc
  tree_cr.cc
  value-closure.cc
  value-cst.cc
  value-seq.cc
  value-str.cc
  value.cc
)

SET_TARGET_PROPERTIES(libzwerg_core PROPERTIES
  COMPILE_FLAGS "-fPIC"
)

# This file is too large for variable tracking assignments, so single
# it out for customization.
ADD_LIBRARY (builtin_dw OBJECT builtin-dw.cc)
SET_TARGET_PROPERTIES(builtin_dw PROPERTIES
  COMPILE_FLAGS "-fno-var-tracking-assignments -fPIC"
)

# libzwerg DSO
ADD_LIBRARY (libzwerg SHARED

  $<TARGET_OBJECTS:builtin_dw>
  $<TARGET_OBJECTS:libzwerg_core>
  atval.cc
  cache.cc
  coverage.cc
  dwcst.cc
  dwfl_context.cc
  dwit.cc
  libzwerg.cc
  value-dw.cc
)

SET (libzwerg_HEADERS

  libzwerg.h)

TARGET_LINK_LIBRARIES (libzwerg ${LIBELF_LIBRARY} ${DWARF_LIBRARIES})

SET_TARGET_PROPERTIES (libzwerg PROPERTIES OUTPUT_NAME "zwerg")
SET_TARGET_PROPERTIES (libzwerg PROPERTIES SOVERSION 0.1)
SET_TARGET_PROPERTIES (libzwerg PROPERTIES LINK_FLAGS
  "-Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/libzwerg.map,--no-undefined")


IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF ()
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL (FILES ${libzwerg_HEADERS} DESTINATION include/libzwerg)
INSTALL (TARGETS libzwerg LIBRARY DESTINATION ${LIB_INSTALL_DIR})

ADD_EXECUTABLE (test-int test-int.cc int.cc)
ADD_TEST (TestInt test-int)

ADD_EXECUTABLE (test-parser

  $<TARGET_OBJECTS:libzwerg_core>
  test-parser.cc
)
ADD_TEST (TestParser test-parser)
