cmake_minimum_required(VERSION 3.8)
project(amr_bringup)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY launch config maps
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(launch_testing_ament_cmake REQUIRED)

  add_launch_test(
    test/test_display_launch.py
    TIMEOUT 30
  )

  add_launch_test(
    test/test_navigation_fixture.py
    TIMEOUT 180
  )

  add_launch_test(
    test/test_missing_tf_fail_closed.py
    TIMEOUT 60
  )
endif()

ament_package()
