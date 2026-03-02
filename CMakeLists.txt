
cmake_minimum_required(VERSION 3.12)

project(libirig106
    VERSION 0.2.0
    LANGUAGES C CXX)

# Build flags
set(CMAKE_BUILD_TYPE RelWithDebInfo)

# Library
include_directories(include)
file(GLOB C_SOURCES "src/*.c")
file(GLOB CXX_SOURCES "src/*.cpp" "src/*.cxx" "src/*.cc")
set(SOURCES ${C_SOURCES} ${CXX_SOURCES})
add_library(irig106 SHARED STATIC ${SOURCES})
set_property(TARGET irig106 PROPERTY C_STANDARD 99)
set_property(TARGET irig106 PROPERTY CXX_STANDARD 11)
set_property(TARGET irig106 PROPERTY POSITION_INDEPENDENT_CODE ON)
install(TARGETS irig106 DESTINATION ${PROJECT_SOURCE_DIR})

# Unity (testing library)
add_subdirectory(tests/unity)
include_directories(tests/unity/src)
include_directories(tests/unity/extras/fixture/src)
include_directories(tests/unity/extras/memory/src)

# Test runner (includes unity and test suite)
file(GLOB C_TESTS "tests/test_*.c")
file(GLOB CXX_TESTS "tests/test_*.cpp" "tests/test_*.cxx" "tests/test_*.cc")
set(TESTS ${C_TESTS} ${CXX_TESTS})
add_executable(test_runner
    tests/all_tests.c
    tests/unity/src/unity.c
    tests/unity/extras/fixture/src/unity_fixture.c
    tests/unity/extras/memory/src/unity_memory.c
    ${TESTS}
)
target_link_libraries(test_runner irig106 unity)
