cmake_minimum_required(VERSION 3.8)
project(ur3e_mrc_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

set(msg_files
  msg/CommandUR3e.msg
  msg/GripperInput.msg
  msg/PositionUR3e.msg
)
rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  "srv/GripperCommand.srv"
)
ament_export_dependencies(rosidl_default_runtime)

ament_package()
