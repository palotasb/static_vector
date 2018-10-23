PROJECT(palotasb_static_vector LANGUAGES CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

add_library(palotasb_static_vector INTERFACE)
target_sources(palotasb_static_vector
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include/palotasb/static_vector.hpp)
target_include_directories(palotasb_static_vector INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(palotasb_static_vector INTERFACE "cxx_std_14")

add_executable(tests tests.cpp)
target_link_libraries(tests palotasb_static_vector)

enable_testing()
add_test(tests tests)
