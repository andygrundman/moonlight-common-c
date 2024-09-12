cmake_minimum_required(VERSION 3.16)
# https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md#foundational-c-support

project(test_moonlight_common_c)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/6dae7eb4a5c3a169f3e298392bff4680224aa94a.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

list(APPEND TESTS
  TestPlatform)

foreach(_test ${TESTS})
  add_executable(${_test} ${_test}.cpp)
  target_include_directories(${_test} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
  target_include_directories(${_test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
  target_include_directories(${_test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../reedsolomon)
  target_link_libraries(${_test} moonlight-common-c enet GTest::gtest_main)
  gtest_discover_tests(${_test})
endforeach()
