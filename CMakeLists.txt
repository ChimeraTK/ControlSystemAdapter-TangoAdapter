PROJECT(ChimeraTK-ControlSystemAdapter-TangoAdapter)
cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 00)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

find_package(PkgConfig)

find_package(ChimeraTK-ApplicationCore 03.03 REQUIRED)
find_package(ChimeraTK-ControlSystemAdapter 02.07 REQUIRED)

pkg_check_modules(TANGO tango REQUIRED )

FIND_PACKAGE(Boost COMPONENTS thread REQUIRED)



include_directories(${CMAKE_SOURCE_DIR}/include ${ChimeraTK-ControlSystemAdapter_INCLUDE_DIRS}  )

GET_DIRECTORY_PROPERTY(output INCLUDE_DIRECTORIES )
MESSAGE(STATUS ${output})

aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)
link_directories(${ChimeraTK-ControlSystemAdapter_LIBRARY_DIRS})
link_directories(${TANGO_LIBRARY_DIRS} )
# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${ChimeraTK-ControlSystemAdapter_CXX_FLAGS} ")
target_link_libraries(${PROJECT_NAME} ${ChimeraTK-ControlSystemAdapter_LIBRARIES} ${TANGO_LIBRARIES})


#===================================================================================================
option(BUILD_EX "Build a test device " ON)
if(BUILD_EX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ChimeraTK-ApplicationCore_CXX_FLAGS}")

    aux_source_directory(${CMAKE_SOURCE_DIR}/examples/example1 exe_sources1)
    include_directories(${CMAKE_SOURCE_DIR}/examples/example1 ${CMAKE_SOURCE_DIR}/include ${ChimeraTK-ControlSystemAdapter_INCLUDE_DIRS} ${TANGO_INCLUDE_DIRS} ${ChimeraTK-ApplicationCore_INCLUDE_DIRS} )

    add_executable(ds_TestChimeraTK ${exe_sources1})

    target_link_libraries(ds_TestChimeraTK ${PROJECT_NAME} ${Boost_SYSTEM_LIBRARY} ${TANGO_LIBRARIES} ${ChimeraTK-ApplicationCore_LIBRARIES})
    file(COPY ${CMAKE_SOURCE_DIR}/examples/example1/configs/example1.dmap DESTINATION ${PROJECT_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/examples/example1/configs/example1.xml DESTINATION ${PROJECT_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/examples/example1/configs/oven.map DESTINATION ${PROJECT_BINARY_DIR})


    #aux_source_directory(${CMAKE_SOURCE_DIR}/examples/example2 exe_sources2)
    #include_directories(${CMAKE_SOURCE_DIR}/examples/example2 ${CMAKE_SOURCE_DIR}/include ${ChimeraTK-ControlSystemAdapter_INCLUDE_DIRS} ${TANGO_INCLUDE_DIRS} ${ChimeraTK-ApplicationCore_INCLUDE_DIRS} )

    #add_executable(ds_ChimeraTKExample2 ${exe_sources2})

    #target_link_libraries(ds_ChimeraTKExample2 ${PROJECT_NAME} ${Boost_SYSTEM_LIBRARY} ${TANGO_LIBRARIES} ${ChimeraTK-ApplicationCore_LIBRARIES})
    #file(COPY ${CMAKE_SOURCE_DIR}/examples/example2/configs/example2.dmap DESTINATION ${PROJECT_BINARY_DIR})
    #file(COPY ${CMAKE_SOURCE_DIR}/examples/example2/configs/oven2.map DESTINATION ${PROJECT_BINARY_DIR})
	#file(COPY ${CMAKE_SOURCE_DIR}/examples/example2/configs/oven2.xlmap DESTINATION ${PROJECT_BINARY_DIR})

endif()



#===================================================================================================

# Install the library and the executables
# this defines architecture-dependent ${CMAKE_INSTALL_LIBDIR}
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# all include files go into include/PROJECT_NAME
# The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
# The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.h"
  PATTERN "${PROJECT_NAME}" EXCLUDE)

# generate cmake config so other projects can find this library
# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)

# we need the public dependencies so create_cmake_config_files can find them as implicit dependencies
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "ChimeraTK-ControlSystemAdapter")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)
