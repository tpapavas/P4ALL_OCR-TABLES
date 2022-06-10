#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "leptonica" for configuration "Debug"
set_property(TARGET leptonica APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(leptonica PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/leptonica-1.82.0d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/leptonica-1.82.0d.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS leptonica )
list(APPEND _IMPORT_CHECK_FILES_FOR_leptonica "${_IMPORT_PREFIX}/debug/lib/leptonica-1.82.0d.lib" "${_IMPORT_PREFIX}/debug/bin/leptonica-1.82.0d.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
