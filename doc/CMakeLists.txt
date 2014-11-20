IF (NOT DEFINED SPHINX_THEME)
    SET (SPHINX_THEME default)
ENDIF ()

IF (NOT DEFINED SPHINX_THEME_DIR)
    SET (SPHINX_THEME_DIR)
ENDIF ()

SET (BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")
SET (SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")
SET (SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

CONFIGURE_FILE ("${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
  		"${BINARY_BUILD_DIR}/conf.py"
		@ONLY)

ADD_CUSTOM_TARGET (doc-html ALL
    ${SPHINX_EXECUTABLE} -q -b html -c "${BINARY_BUILD_DIR}"
	-d "${SPHINX_CACHE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" "${SPHINX_HTML_DIR}"
    COMMENT "Building HTML documentation")

ADD_CUSTOM_TARGET (GenerateDocs ALL
  ../libzwerg/dwgrep-gendoc > vocabulary.rst
  DEPENDS ../libzwerg/dwgrep-gendoc
  COMMENT "Building vocabulary reference")

ADD_CUSTOM_TARGET (doc)
ADD_DEPENDENCIES (doc doc-html)
ADD_DEPENDENCIES (doc-html GenerateDocs)
