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

# Find OpenMP (special handling for macOS with Homebrew libomp)
if(APPLE)
    # Try to find libomp from Homebrew
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(LIBOMP_PREFIX)
        set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
        set(OpenMP_C_LIB_NAMES "omp")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "${LIBOMP_PREFIX}/lib/libomp.dylib")
        set(OpenMP_CXX_FLAGS "${OpenMP_CXX_FLAGS}" CACHE STRING "" FORCE)
        set(OpenMP_C_FLAGS "${OpenMP_C_FLAGS}" CACHE STRING "" FORCE)
    endif()
endif()

find_package(OpenMP REQUIRED)

# Library sources
set(GAIALIB_SOURCES
    src/types.cpp
    src/gaia_cache.cpp
    src/gaia_client.cpp
    src/gaia_mag18_catalog.cpp
    src/gaia_mag18_catalog_v2.cpp
    src/concurrent_multifile_catalog_v2.cpp
    src/unified_gaia_catalog.cpp
    src/common_star_names.cpp
    src/iau_star_catalog_parser.cpp
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
        OpenMP::OpenMP_CXX
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

# Install library and headers
install(TARGETS ioc_gaialib
    EXPORT IOC_GaiaLibTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install CMake config files
include(CMakePackageConfigHelpers)

# Generate config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/IOC_GaiaLibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/IOC_GaiaLibConfig.cmake"
    INSTALL_DESTINATION lib/cmake/IOC_GaiaLib
)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/IOC_GaiaLibConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/IOC_GaiaLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/IOC_GaiaLibConfigVersion.cmake"
    DESTINATION lib/cmake/IOC_GaiaLib
)

# Install targets
install(EXPORT IOC_GaiaLibTargets
    FILE IOC_GaiaLibTargets.cmake
    DESTINATION lib/cmake/IOC_GaiaLib
)
