ADD_LIBRARY (AuxLib OBJECT
  ../libzwerg/strip.cc
  options.cc)

ADD_EXECUTABLE (dwgrep dwgrep.cc $<TARGET_OBJECTS:AuxLib>)
ADD_EXECUTABLE (dwgrep-genman genman.cc $<TARGET_OBJECTS:AuxLib>)
INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR})
TARGET_LINK_LIBRARIES (dwgrep libzwerg)
