FLEX_TARGET (Lexer lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
BISON_TARGET (Parser parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)

ADD_LIBRARY (libzwerg SHARED

  libzwerg.cc
  atval.cc
  build.cc
  builtin-closure.cc
  builtin-cmp.cc
  builtin-cst.cc
  builtin-dw.cc
  builtin-shf.cc
  builtin.cc
  cache.cc
  constant.cc
  coverage.cc
  dwcst.cc
  dwfl_context.cc
  dwit.cc
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
  value-dw.cc
  value-seq.cc
  value-str.cc
  value.cc
)

SET (libzwerg_HEADERS

  libzwerg.h)

TARGET_LINK_LIBRARIES (libzwerg ${LIBELF_LIBRARY} ${DWARF_LIBRARIES})

SET_TARGET_PROPERTIES (libzwerg PROPERTIES OUTPUT_NAME "zwerg")
SET_TARGET_PROPERTIES (libzwerg PROPERTIES SOVERSION 1)

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF ()
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL (FILES ${libzwerg_HEADERS} DESTINATION include/libzwerg)
INSTALL (TARGETS libzwerg LIBRARY DESTINATION ${LIB_INSTALL_DIR})

ADD_EXECUTABLE (test-int test-int.cc int.cc)
ADD_TEST (TestInt test-int)

ADD_EXECUTABLE (test-parser

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
  test-parser.cc
  tree.cc
  tree_cr.cc
  value-closure.cc
  value-cst.cc
  value-seq.cc
  value-str.cc
  value.cc
)
ADD_TEST (TestParser test-parser)
