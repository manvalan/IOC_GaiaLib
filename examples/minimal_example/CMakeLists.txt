cmake_minimum_required(VERSION 3.15)
project(MinimalGaiaApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find IOC_GaiaLib (assumes library is installed system-wide)
find_package(IOC_GaiaLib REQUIRED)

# Create executable
add_executable(minimal_app main.cpp)

# Link library
target_link_libraries(minimal_app PRIVATE IOC_GaiaLib::ioc_gaialib)
