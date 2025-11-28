cmake_minimum_required(VERSION 3.15)

# Example programs - using the unified API only

add_executable(unified_api_demo unified_api_demo.cpp)
target_link_libraries(unified_api_demo PRIVATE ioc_gaialib)

add_executable(iau_integration_test iau_integration_test.cpp)
target_link_libraries(iau_integration_test PRIVATE ioc_gaialib)

add_executable(integration_test integration_test.cpp)
target_link_libraries(integration_test PRIVATE ioc_gaialib)

# Install examples
install(TARGETS unified_api_demo iau_integration_test integration_test
    RUNTIME DESTINATION bin
)
