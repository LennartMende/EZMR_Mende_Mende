cmake_minimum_required(VERSION 3.8)
project(sensors)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(robot_interfaces REQUIRED)

add_library(sensors_lib
  src/sonar_front_subscriber.cpp
  src/sonar_left_subscriber.cpp
  src/bumpers_subscriber.cpp
  src/imu_subscriber.cpp
)

ament_target_dependencies(sensors_lib
  rclcpp
  std_msgs
  robot_interfaces
)

target_include_directories(sensors_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS sensors_lib
  EXPORT export_sensors_lib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  EXPORT export_sensors_lib
  DESTINATION share/${PROJECT_NAME}/cmake
)

ament_export_targets(export_sensors_lib HAS_LIBRARY_TARGET)

ament_export_dependencies(
  rclcpp
  std_msgs
  robot_interfaces
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
