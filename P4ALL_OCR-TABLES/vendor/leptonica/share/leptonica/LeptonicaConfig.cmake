# ===================================================================================
#  The Leptonica CMake configuration file
#
#             ** File generated automatically, do not modify **
#
#  Usage from an external project:
#    In your CMakeLists.txt, add these lines:
#
#    find_package(Leptonica REQUIRED)
#    include_directories(${Leptonica_INCLUDE_DIRS})
#    target_link_libraries(MY_TARGET_NAME ${Leptonica_LIBRARIES})
#
#    This file will define the following variables:
#      - Leptonica_LIBRARIES             : The list of all imported targets for OpenCV modules.
#      - Leptonica_INCLUDE_DIRS          : The Leptonica include directories.
#      - Leptonica_VERSION               : The version of this Leptonica build: "1.82.0"
#      - Leptonica_VERSION_MAJOR         : Major version part of Leptonica_VERSION: "1"
#      - Leptonica_VERSION_MINOR         : Minor version part of Leptonica_VERSION: "82"
#      - Leptonica_VERSION_PATCH         : Patch version part of Leptonica_VERSION: "0"
#
# ===================================================================================
include(CMakeFindDependencyMacro)
find_dependency(JPEG)
find_dependency(ZLIB)
find_dependency(PNG)
find_dependency(TIFF)
find_dependency(GIF)
find_dependency(WebP)
find_dependency(OpenJPEG)

include(${CMAKE_CURRENT_LIST_DIR}/LeptonicaTargets.cmake)

# ======================================================
#  Version variables:
# ======================================================

SET(Leptonica_VERSION           1.82.0)
SET(Leptonica_VERSION_MAJOR     1)
SET(Leptonica_VERSION_MINOR     82)
SET(Leptonica_VERSION_PATCH     0)

# ======================================================
# Include directories to add to the user project:
# ======================================================

# Provide the include directories to the caller
get_filename_component(Leptonica_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../include" ABSOLUTE)
list(APPEND Leptonica_INCLUDE_DIRS "${Leptonica_INCLUDE_DIRS}/leptonica")

# ====================================================================
# Link libraries:
# ====================================================================

set(Leptonica_LIBRARIES         leptonica)
