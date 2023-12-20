# Note: This CMakeLists.txt is a minimal complete example how to use the
# *installed* application core library. It is not included from the main CMakeLists.txt
# which is used to build ApplicationCore itself.
project(TestChimeraTK)

cmake_minimum_required(VERSION 3.16)

# Use the project template to get the settings required for an application core project
# The Version 01.23.45 does not represent the actual version of the example and is used
# only for demonstration purposes. Please change when using the example as a template.
set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)
include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)

# Add the dependencies. We need ApplicationCore and a control system adapter implementation.
find_package(ChimeraTK-ApplicationCore 03.03 REQUIRED)
find_package(ChimeraTK-ControlSystemAdapter-TangoAdapter REQUIRED)

find_package(PkgConfig)
pkg_check_modules(TANGO tango REQUIRED )


link_directories(${TANGO_LIBRARY_DIRS} )

aux_source_directory(${CMAKE_SOURCE_DIR} src)
include_directories(${CMAKE_SOURCE_DIR}  )


# Server executable: link the server library with the application factory and the tango control system adapter
add_executable(ds_${PROJECT_NAME} ${src})
set_target_properties(ds_${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${ChimeraTK-ApplicationCore_CXX_FLAGS} ")

target_include_directories(ds_${PROJECT_NAME} PRIVATE ${TANGO_INCLUDE_DIRS} )
target_link_libraries(ds_${PROJECT_NAME} PRIVATE  ${TANGO_LIBRARIES} ${ChimeraTK-ControlSystemAdapter-TangoAdapter_LIBRARIES} ${ChimeraTK-ApplicationCore_LIBRARIES})

# copy the (test) config files to the build directory for tests
file(COPY config/ DESTINATION ${PROJECT_BINARY_DIR})



# Do not install the config. It is only a test config.
# The real config will come from a config generator and usually depends on the instance.
