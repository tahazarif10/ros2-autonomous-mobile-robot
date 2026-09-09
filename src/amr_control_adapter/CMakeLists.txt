cmake_minimum_required(VERSION 3.24)
project(amr_control_adapter)

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(lifecycle_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)

include(FetchContent)

set(RCC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RCC_ENABLE_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(RCC_ENABLE_SANITIZERS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  robotics_control_core
  GIT_REPOSITORY https://github.com/tahazarif10/robotics-control-core.git
  GIT_TAG dda918bb362c5aa79f025a8f89664a215bb5a33c
  GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(robotics_control_core)

add_library(amr_control_adapter_lib
  src/control_policy.cpp
  src/control_adapter_node.cpp
)
target_compile_features(amr_control_adapter_lib PUBLIC cxx_std_20)
target_include_directories(amr_control_adapter_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(
  amr_control_adapter_lib
  geometry_msgs
  nav_msgs
  lifecycle_msgs
  rclcpp
  rclcpp_lifecycle
)
target_link_libraries(amr_control_adapter_lib robotics::control)

add_executable(amr_control_adapter_node src/main.cpp)
target_compile_features(amr_control_adapter_node PUBLIC cxx_std_20)
target_link_libraries(amr_control_adapter_node amr_control_adapter_lib)
ament_target_dependencies(amr_control_adapter_node rclcpp rclcpp_lifecycle)

install(
  TARGETS amr_control_adapter_lib amr_control_adapter_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)
install(
  DIRECTORY include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(control_policy_test test/test_control_policy.cpp)
  target_compile_features(control_policy_test PUBLIC cxx_std_20)
  target_link_libraries(control_policy_test amr_control_adapter_lib)

  ament_add_gtest(control_adapter_lifecycle_test test/test_control_adapter_lifecycle.cpp)
  target_compile_features(control_adapter_lifecycle_test PUBLIC cxx_std_20)
  target_link_libraries(control_adapter_lifecycle_test amr_control_adapter_lib)
  ament_target_dependencies(
    control_adapter_lifecycle_test
    lifecycle_msgs
    rclcpp
    rclcpp_lifecycle
  )
endif()

ament_package()
