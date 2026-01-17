cmake_minimum_required(VERSION 3.20)
project(smart_ptrs LANGUAGES CXX)

option(SMART_PTRS_BUILD_TESTS "Build tests" ON)

add_library(smart_ptrs INTERFACE)
target_include_directories(smart_ptrs INTERFACE ${PROJECT_SOURCE_DIR})
target_compile_features(smart_ptrs INTERFACE cxx_std_20)

if(SMART_PTRS_BUILD_TESTS)
  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(test_shared_ptr shared-from-this/test.cpp)
  target_link_libraries(test_shared_ptr PRIVATE smart_ptrs Catch2::Catch2WithMain)

  add_executable(test_weak_ptr shared-from-this/test_weak.cpp)
  target_link_libraries(test_weak_ptr PRIVATE smart_ptrs Catch2::Catch2WithMain)

  add_executable(test_unique_ptr shared-from-this/test_unique.cpp)
  target_link_libraries(test_unique_ptr PRIVATE smart_ptrs Catch2::Catch2WithMain)

  include(CTest)
  include(Catch)
  catch_discover_tests(test_shared_ptr)
  catch_discover_tests(test_weak_ptr)
  catch_discover_tests(test_unique_ptr)
endif()
