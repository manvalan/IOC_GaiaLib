cmake_minimum_required(VERSION 3.15)

# Example programs
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE ioc_gaialib)

add_executable(integration_test integration_test.cpp)
target_link_libraries(integration_test PRIVATE ioc_gaialib)

add_executable(real_data_test real_data_test.cpp)
target_link_libraries(real_data_test PRIVATE ioc_gaialib)

# Install examples
install(TARGETS simple_test integration_test real_data_test
    RUNTIME DESTINATION bin
)
