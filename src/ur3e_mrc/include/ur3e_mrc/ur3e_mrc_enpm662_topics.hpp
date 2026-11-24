#include <chrono>
#include <memory>
#include <functional>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "boost/math/constants/constants.hpp"

#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/bool.hpp"

#include "ur_msgs/msg/io_states.hpp"

#include "ur3e_mrc_msgs/msg/position_ur3e.hpp"
#include "ur3e_mrc_msgs/msg/gripper_input.hpp"