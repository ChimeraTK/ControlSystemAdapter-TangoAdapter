project(ChimeraTK-ControlSystemAdapter-TangoAdapter)
cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

include(cmake/set_default_build_to_release.cmake)
include(cmake/add_linter_target.cmake)
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)
include(cmake/enable_doxygen_documentation.cmake)

find_package(PkgConfig)

# Add the dependencies
find_package(ChimeraTK-ControlSystemAdapter 02.07 REQUIRED)
pkg_check_modules(TANGO REQUIRED IMPORTED_TARGET tango)

include_directories(${CMAKE_SOURCE_DIR}/include)

aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

link_directories(${TANGO_LIBRARY_DIRS})

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources})

# exported includes are all under ${CMAKE_INSTALL_PREFIX}/include
target_include_directories(${PROJECT_NAME}
  INTERFACE "$<INSTALL_INTERFACE:include>")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME}
  PUBLIC ChimeraTK::ChimeraTK-ControlSystemAdapter)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::TANGO)

# do not remove runtime paths of the library when installing (helps for unsually
# located implicit dependencies)
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

# Install the library and the executables this defines architecture-dependent
# ${CMAKE_INSTALL_LIBDIR}
include(GNUInstallDirs)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# all include files go into include/PROJECT_NAME The exclusion of
# ${PROJECT_NAME} prevents the recursive installation of the files just being
# installed. The original headers are in include/*.h, the installed ones in
# include/ChimeraTK/*.h.
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/
  DESTINATION include/ChimeraTK
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "${PROJECT_NAME}" EXCLUDE)

# generate cmake config so other projects can find this library we support our
# cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)

# we need the public dependencies so create_cmake_config_files can find them as
# implicit dependencies
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES
  "ChimeraTK-ControlSystemAdapter")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)
