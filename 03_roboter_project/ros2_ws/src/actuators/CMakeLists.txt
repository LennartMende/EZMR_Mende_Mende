cmake_minimum_required(VERSION 3.8)
project(actuators)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(robot_interfaces REQUIRED)

include_directories(include)


# =========================
# Library
# =========================

add_library(actuators_lib
  src/motors_publisher.cpp
  src/led_publisher.cpp
  src/buzzer_publisher.cpp
  src/oled_publisher.cpp
)

ament_target_dependencies(actuators_lib
  rclcpp
  std_msgs
  geometry_msgs
  robot_interfaces
)

target_include_directories(actuators_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)


# =========================
# Executables
# =========================

add_executable(cmd_vel_publisher
  src/cmd_vel_publisher.cpp
)

ament_target_dependencies(cmd_vel_publisher
  rclcpp
  geometry_msgs
)


# add_executable(buzzer_publisher
#   src/buzzer_publisher.cpp
# )

# ament_target_dependencies(buzzer_publisher
#   rclcpp
#   std_msgs
#   robot_interfaces
# )


# add_executable(led_publisher
#   src/led_publisher.cpp
# )

# ament_target_dependencies(led_publisher
#   rclcpp
#   std_msgs
#   robot_interfaces
# )


# add_executable(motors_publisher
#   src/motors_publisher.cpp
# )

# ament_target_dependencies(motors_publisher
#   rclcpp
#   std_msgs
#   robot_interfaces
# )


# add_executable(oled_publisher
#   src/oled_publisher.cpp
# )

# ament_target_dependencies(oled_publisher
#   rclcpp
#   std_msgs
#   robot_interfaces
# )


# =========================
# Install
# =========================

install(
  TARGETS
    actuators_lib
    cmd_vel_publisher
    # buzzer_publisher
    # led_publisher
    # motors_publisher
    # oled_publisher
  EXPORT actuators_targets
  DESTINATION lib/${PROJECT_NAME}
)


install(
  DIRECTORY include/
  DESTINATION include
)


# =========================
# Export library
# =========================

ament_export_targets(actuators_targets HAS_LIBRARY_TARGET)

ament_export_dependencies(
  rclcpp
  std_msgs
  geometry_msgs
  robot_interfaces
)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif()


ament_package()