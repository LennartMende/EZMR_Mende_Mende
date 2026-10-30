cmake_minimum_required(VERSION 3.8)
project(algorithm)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(robot_interfaces REQUIRED)

find_package(actuators REQUIRED)
find_package(sensors REQUIRED)

add_executable(robot_controller
  src/robot_controller_main.cpp
  src/robot_controller.cpp
  src/pledge_algorithm.cpp
)

ament_target_dependencies(robot_controller
  rclcpp
  std_msgs
  geometry_msgs
  robot_interfaces
)

target_link_libraries(robot_controller
  actuators::actuators_lib
  sensors::sensors_lib
)

target_include_directories(robot_controller PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(TARGETS robot_controller
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
