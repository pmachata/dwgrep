INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})

FLEX_TARGET (Lexer lexer.ll
  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc
  COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.hh)

BISON_TARGET (Parser parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)

ADD_FLEX_BISON_DEPENDENCY (Lexer Parser)

ADD_CUSTOM_COMMAND (OUTPUT known-dwarf.h
  COMMAND gawk -f
  	${CMAKE_SOURCE_DIR}/known-dwarf.awk
	${DWARF_INCLUDE_DIR}/dwarf.h > known-dwarf.h
  DEPENDS ${CMAKE_SOURCE_DIR}/known-dwarf.awk ${DWARF_INCLUDE_DIR}/dwarf.h
)

# Core pseudo-library.
ADD_LIBRARY (LibzwergCore OBJECT

  ${BISON_Parser_OUTPUTS}
  ${FLEX_Lexer_OUTPUTS}
  known-dwarf.h
  build.cc
  builtin-closure.cc
  builtin-cmp.cc
  builtin-cst.cc
  builtin-shf.cc
  builtin.cc
  constant.cc
  docstring.cc
  init.cc
  int.cc
  op.cc
  overload.cc
  selector.cc
  stack.cc
  strip.cc
  tree.cc
  tree_cr.cc
  value-closure.cc
  value-cst.cc
  value-seq.cc
  value-str.cc
  value.cc
)

SET_TARGET_PROPERTIES (LibzwergCore PROPERTIES
  COMPILE_FLAGS "-fPIC"
)

# Dwarf pseudo-library.
ADD_LIBRARY (LibzwergDw OBJECT

  known-dwarf.h
  atval.cc
  cache.cc
  coverage.cc
  dwcst.cc
  dwfl_context.cc
  dwit.cc
  dwmods.cc
  libzwerg.cc
  value-dw.cc
  builtin-dw-abbrev.cc
)

SET_TARGET_PROPERTIES (LibzwergDw PROPERTIES
  COMPILE_FLAGS "-fPIC"
)

# This file is too large for variable tracking assignments, so single
# it out for customization.
ADD_LIBRARY (BuiltinDw OBJECT builtin-dw.cc known-dwarf.h)
SET_TARGET_PROPERTIES (BuiltinDw PROPERTIES
  COMPILE_FLAGS "-fno-var-tracking-assignments -fPIC"
)

SET (LibzwergAll

  $<TARGET_OBJECTS:LibzwergCore>
  $<TARGET_OBJECTS:LibzwergDw>
  $<TARGET_OBJECTS:BuiltinDw>
)

# libzwerg DSO
ADD_LIBRARY (libzwerg SHARED ${LibzwergAll})

SET (libzwerg_HEADERS libzwerg.h)

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

  $<TARGET_OBJECTS:LibzwergCore>
  test-parser.cc
)
ADD_TEST (TestParser test-parser)

IF (GTEST_FOUND)
  SET (TESTCASE_DIR "--test-case-directory=${CMAKE_SOURCE_DIR}/tests/")

  ADD_LIBRARY (TestStub OBJECT test-stub.cc)

  ADD_EXECUTABLE (test-dw test-dw.cc $<TARGET_OBJECTS:TestStub> ${LibzwergAll})
  TARGET_LINK_LIBRARIES (test-dw
    ${GTEST_LIBRARIES} ${LIBELF_LIBRARY} ${DWARF_LIBRARIES})
  ADD_TEST (TestDw test-dw ${TESTCASE_DIR})

  ADD_EXECUTABLE (test-value-cst test-value-cst.cc
    $<TARGET_OBJECTS:TestStub> $<TARGET_OBJECTS:LibzwergCore>)
  TARGET_LINK_LIBRARIES (test-value-cst ${GTEST_LIBRARIES})
  ADD_TEST (TestValueCst test-value-cst ${TESTCASE_DIR})

  ADD_EXECUTABLE (test-builtin-cmp test-builtin-cmp.cc
    $<TARGET_OBJECTS:TestStub> $<TARGET_OBJECTS:LibzwergCore>)
  TARGET_LINK_LIBRARIES (test-builtin-cmp ${GTEST_LIBRARIES})
  ADD_TEST (TestBuiltinCmp test-builtin-cmp ${TESTCASE_DIR})

  ADD_EXECUTABLE (test-coverage test-coverage.cc coverage.cc
    $<TARGET_OBJECTS:TestStub>)
  TARGET_LINK_LIBRARIES (test-coverage ${GTEST_LIBRARIES})
  ADD_TEST (TestBuiltinCovegare test-coverage ${TESTCASE_DIR})
ENDIF ()

IF (SPHINX_EXECUTABLE)
  ADD_EXECUTABLE (dwgrep-gendoc dwgrep-gendoc.cc ${LibzwergAll})
  TARGET_LINK_LIBRARIES (dwgrep-gendoc
    ${LIBELF_LIBRARY} ${DWARF_LIBRARIES} -ldl)
ENDIF ()
