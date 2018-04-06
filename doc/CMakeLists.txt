SET (DocSources

  cli.rst
  fulltoc.rst
  index.rst
  syntax.rst
  tutorial.rst
  examples.rst
  zw_vocabulary_core-blurb.txt
  zw_vocabulary_dwarf-blurb.txt
  zw_vocabulary_elf-blurb.txt
)

IF (NOT DEFINED SPHINX_THEME)
  SET (SPHINX_THEME default)
ENDIF ()

IF (NOT DEFINED SPHINX_THEME_DIR)
  SET (SPHINX_THEME_DIR)
ENDIF ()

SET (AUX_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_source")
SET (BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")
SET (SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")
SET (SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")
SET (SPHINX_MAN_DIR "${CMAKE_CURRENT_BINARY_DIR}/man")

FOREACH (SOURCE ${DocSources})
  CONFIGURE_FILE ("${SOURCE}" "${AUX_SOURCE_DIR}/${SOURCE}" COPYONLY)
ENDFOREACH ()

CONFIGURE_FILE ("${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
  		"${BINARY_BUILD_DIR}/conf.py"
		@ONLY)

ADD_CUSTOM_TARGET (GenerateDocs
  COMMAND ../libzwerg/dwgrep-gendoc
	zw_vocabulary_core "Core vocabulary"
	> "${AUX_SOURCE_DIR}/vocabulary-core.rst"

  COMMAND ../libzwerg/dwgrep-gendoc
	zw_vocabulary_dwarf "Dwarf vocabulary"
	> "${AUX_SOURCE_DIR}/vocabulary-dwarf.rst"

  COMMAND ../libzwerg/dwgrep-gendoc
	zw_vocabulary_elf "Elf vocabulary"
	> "${AUX_SOURCE_DIR}/vocabulary-elf.rst"

  DEPENDS ../libzwerg/dwgrep-gendoc
  COMMENT "Building vocabulary reference")

ADD_CUSTOM_TARGET (GenerateOptions
  COMMAND ../dwgrep/dwgrep-genman
	> "${AUX_SOURCE_DIR}/options.txt"

  DEPENDS ../dwgrep/dwgrep-genman
  COMMENT "Building CLI option reference")

ADD_CUSTOM_TARGET (doc-html
    ${SPHINX_EXECUTABLE} -q -b html -c "${BINARY_BUILD_DIR}"
	-d "${SPHINX_CACHE_DIR}" "${AUX_SOURCE_DIR}" "${SPHINX_HTML_DIR}"
    COMMENT "Building HTML documentation")

ADD_CUSTOM_TARGET (doc-man
    ${SPHINX_EXECUTABLE} -q -b man -c "${BINARY_BUILD_DIR}"
	-d "${SPHINX_CACHE_DIR}" "${AUX_SOURCE_DIR}" "${SPHINX_MAN_DIR}"
    COMMENT "Building manual pages")

INSTALL (FILES ${SPHINX_MAN_DIR}/dwgrep.1
  DESTINATION share/man/man1 OPTIONAL)

ADD_CUSTOM_TARGET (doc)
ADD_DEPENDENCIES (doc doc-html doc-man)

ADD_CUSTOM_TARGET (doc-gen)
ADD_DEPENDENCIES (doc-gen GenerateDocs GenerateOptions)

ADD_DEPENDENCIES (doc-html doc-gen)
ADD_DEPENDENCIES (doc-man doc-gen)
