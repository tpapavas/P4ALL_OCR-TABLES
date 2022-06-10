#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libtesseract" for configuration "Debug"
set_property(TARGET libtesseract APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libtesseract PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/tesseract41d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/tesseract41d.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS libtesseract )
list(APPEND _IMPORT_CHECK_FILES_FOR_libtesseract "${_IMPORT_PREFIX}/debug/lib/tesseract41d.lib" "${_IMPORT_PREFIX}/debug/bin/tesseract41d.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
