cmake_minimum_required(VERSION 3.15)

# Example programs
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE ioc_gaialib)

add_executable(integration_test integration_test.cpp)
target_link_libraries(integration_test PRIVATE ioc_gaialib)

add_executable(real_data_test real_data_test.cpp)
target_link_libraries(real_data_test PRIVATE ioc_gaialib)

add_executable(compile_catalog compile_catalog.cpp)
target_link_libraries(compile_catalog PRIVATE ioc_gaialib)

add_executable(test_catalog_query test_catalog_query.cpp)
target_link_libraries(test_catalog_query PRIVATE ioc_gaialib)

add_executable(quick_catalog_test quick_catalog_test.cpp)
target_link_libraries(quick_catalog_test PRIVATE ioc_gaialib)

add_executable(test_mag17 test_mag17.cpp)
target_link_libraries(test_mag17 PRIVATE ioc_gaialib)

add_executable(test_grappa test_grappa.cpp)
target_link_libraries(test_grappa PRIVATE ioc_gaialib)

# Install examples
install(TARGETS simple_test integration_test real_data_test compile_catalog test_catalog_query quick_catalog_test test_mag17 test_grappa
    RUNTIME DESTINATION bin
)
