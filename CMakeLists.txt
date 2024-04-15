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

# For some reason the system library has to be mentioned explicitly to the
# linker. This is strange because the thead library has it as a dependency.
option(BUILD_TESTS "Build test programs" ON)

if(BUILD_TESTS)
  find_package(
    Boost
    COMPONENTS unit_test_framework
    REQUIRED)
  enable_testing()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/include/*.h")
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} ${headers})

# exported includes are all under ${CMAKE_INSTALL_PREFIX}/include
target_include_directories(${PROJECT_NAME}
                           INTERFACE "$<INSTALL_INTERFACE:include>")
set_target_properties(
  ${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
                             SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME}
                      PUBLIC ChimeraTK::ChimeraTK-ControlSystemAdapter)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::TANGO)

# do not remove runtime paths of the library when installing (helps for unsually
# located implicit dependencies)
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

# Create the executables for automated unit testing. Currently we assume that
# they are all tests contained in one file, so each file gives a new executable.
# This section has to be adapted if this should change.
if(BUILD_TESTS)
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/src_lib testLibrarySources)
  file(GLOB testLibraryHeaders ${CMAKE_SOURCE_DIR}/tests/include_lib/*.h)

  add_library(TestLib STATIC ${testLibrarySources} ${testLibraryHeaders})
  target_link_libraries(TestLib PUBLIC ChimeraTK::ChimeraTK-ControlSystemAdapter PkgConfig::TANGO )
  target_include_directories(TestLib PUBLIC ${CMAKE_SOURCE_DIR}/tests/include_lib)
  # loop over all sources files in execultables_src directory
  aux_source_directory(${CMAKE_SOURCE_DIR}/tests/src testExecutables)

  foreach(testExecutableSrcFile ${testExecutables} ${referenceExecutables})
    # NAME_WE means the base name without path and (longest) extension
    get_filename_component(executableName ${testExecutableSrcFile} NAME_WE)

    # each test includes a DOOCS server
    add_executable(${executableName} ${testExecutableSrcFile})
    target_include_directories(${executableName}
                               PRIVATE ${CMAKE_SOURCE_DIR}/tests/include)

    # need to force no-as-needed otherwise the class_factory function from the
    # adapter will  be discarded and the tango server will not start
    target_link_options(${executableName} PUBLIC "-Wl,--no-as-needed")
    target_link_libraries(${executableName} PUBLIC ${PROJECT_NAME}
                                                   TestLib
                                                   PkgConfig::TANGO)
    add_test(${executableName} ${executableName})
  endforeach(testExecutableSrcFile)

  # Set up the test pairs that produce and read stored attributes
  set_tests_properties(testTangoServerPersistSpectrum PROPERTIES FIXTURES_SETUP SpectrumPersistDb)
  set_tests_properties(testTangoServerPersistSpectrumRead PROPERTIES FIXTURES_SETUP SpectrumPersistDb)

  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/testPersistSpectrumDatabase_template.db DESTINATION ${CMAKE_BINARY_DIR}/tests)
endif()

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
