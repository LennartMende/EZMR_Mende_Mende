cmake_minimum_required(VERSION 3.8)
project(turtlesim_task)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
add_executable(turtlesim_publisher src/turtlesim_publisher.cpp)
ament_target_dependencies(turtlesim_publisher rclcpp geometry_msgs)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

install(TARGETS
  turtlesim_publisher
  DESTINATION lib/${PROJECT_NAME})

ament_package()
