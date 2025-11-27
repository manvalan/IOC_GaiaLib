cmake_minimum_required(VERSION 3.15)
project(IOC_GaiaLib VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_DOCS "Build documentation" OFF)

# Find dependencies
find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

# Library sources
set(GAIALIB_SOURCES
    src/types.cpp
    src/gaia_cache.cpp
    src/gaia_client.cpp
    src/catalog_compiler.cpp
    src/grappa_reader.cpp
)

# Create library
add_library(ioc_gaialib STATIC ${GAIALIB_SOURCES})

target_include_directories(ioc_gaialib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CURL_INCLUDE_DIRS}
        ${LIBXML2_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(ioc_gaialib
    PUBLIC
        ${CURL_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        Threads::Threads
        ZLIB::ZLIB
)

# Compiler warnings
if(MSVC)
    target_compile_options(ioc_gaialib PRIVATE /W4)
else()
    target_compile_options(ioc_gaialib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    # TODO: add_subdirectory(tests)
endif()

# Install
install(TARGETS ioc_gaialib
    EXPORT IOC_GaiaLibTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

install(EXPORT IOC_GaiaLibTargets
    FILE IOC_GaiaLibConfig.cmake
    NAMESPACE IOC::
    DESTINATION lib/cmake/IOC_GaiaLib
)
