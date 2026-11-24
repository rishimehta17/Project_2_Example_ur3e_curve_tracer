cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)
project(mrc_gazebo_plugins)


# find dependencies
find_package(ament_cmake REQUIRED)

find_package(ignition-gazebo6 REQUIRED)
set(IGN_GAZEBO_VER ${ignition-gazebo6_VERSION_MAJOR})
find_package(ignition-plugin1 REQUIRED COMPONENTS register)
set(IGN_PLUGIN_VER ${ignition-plugin1_VERSION_MAJOR})
find_package(ignition-common4 REQUIRED COMPONENTS graphics)
set(IGN_COMMON_VER ${ignition-common4_VERSION_MAJOR})

# find_package(ignition-fuel_tools7 REQUIRED)
# find_package(ignition-math6 REQUIRED)
# set(IGN_MATH_VER ${ignition-math6_VERSION_MAJOR})
# find_package(ignition-msgs8 REQUIRED)
# find_package(ignition-transport11 REQUIRED)
# set(IGN_TRANSPORT_VER ${ignition-transport11_VERSION_MAJOR})
# find_package(ignition-rendering6 REQUIRED)
# set(IGN_RENDERING_VER ${ignition-rendering6_VERSION_MAJOR})
# find_package(ignition-cmake2 REQUIRED)

find_package(sdformat12 REQUIRED)
find_package(std_msgs REQUIRED)


add_library(VacuumGripper SHARED src/VacuumGripper.cpp)
set_property(TARGET VacuumGripper PROPERTY CXX_STANDARD 17)
target_link_libraries(VacuumGripper PUBLIC
  ignition-gazebo${IGN_GAZEBO_VER}::core
  ignition-plugin${IGN_PLUGIN_VER}::ignition-plugin${IGN_PLUGIN_VER}
  ignition-gazebo${IGN_GAZEBO_VER}::ignition-gazebo${IGN_GAZEBO_VER}
  ignition-common${IGN_COMMON_VER}::ignition-common${IGN_COMMON_VER}
)

install(
  TARGETS
    VacuumGripper
  DESTINATION lib
)

ament_package()
