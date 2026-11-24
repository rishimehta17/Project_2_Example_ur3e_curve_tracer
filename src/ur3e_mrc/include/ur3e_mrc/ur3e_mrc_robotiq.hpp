#include <thread>
#include <chrono>
#include <memory>
#include <functional>

#include "rclcpp/rclcpp.hpp"

#include "std_msgs/msg/bool.hpp"

#include <ur_rtde/robotiq_gripper.h>

#include "ur3e_mrc_msgs/srv/gripper_command.hpp"