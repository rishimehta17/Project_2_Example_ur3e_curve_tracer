#include <ur3e_mrc/ur3e_mrc_enpm662_ctrl.hpp>

#define N_JOINTS 6
#define CTRL_TO_RUN "scaled_joint_trajectory_controller"

bool volatile grip_isON = false;
bool volatile laserP_isON = false;
bool volatile ur3e_isReady = false;

namespace cb_group_ur3e
{
class UR3eENPM662Control
{
public:
  explicit UR3eENPM662Control(rclcpp::Node::SharedPtr node) : node_(node)
  {
    client_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    sub_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    io_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    lp_cb_group_ = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);

    client_traj_ = rclcpp_action::create_client<control_msgs::action::FollowJointTrajectory>(node_, "scaled_joint_trajectory_controller/follow_joint_trajectory", client_cb_group_);
    RCLCPP_INFO(node_->get_logger(), "Waiting for the joint_trajectory_action server");

    bool init_status_ = false;
    init_status_ = client_traj_->wait_for_action_server(std::chrono::seconds(20));
    if (init_status_)
      RCLCPP_INFO(node_->get_logger(), "Connected to the joint_trajectory_action server");
    else
      RCLCPP_ERROR(node_->get_logger(), "Failed to connect to the joint_trajectory_action server");

    rclcpp::SubscriptionOptions options_comm;
    options_comm.callback_group = sub_cb_group_;
    sub_comm_ = node_->create_subscription<ur3e_mrc_msgs::msg::CommandUR3e>("ur3e/command", 10, std::bind(&UR3eENPM662Control::comm_callback, this, std::placeholders::_1), options_comm);
    RCLCPP_INFO(node_->get_logger(), "Subscribed to ur3e command");

    rclcpp::SubscriptionOptions options_lp;
    options_lp.callback_group = lp_cb_group_;
    sub_lp_ = node_->create_subscription<std_msgs::msg::Bool>("ur3e/laser_point", 10, std::bind(&UR3eENPM662Control::lp_callback, this, std::placeholders::_1), options_lp);
    RCLCPP_INFO(node_->get_logger(), "Subscribed to ur3e laser point ");    

    // // Controller manager service to switch controllers
    // controller_manager_srv_ = node_->create_client<controller_manager_msgs::srv::SwitchController>("controller_manager/switch_controller");
    // // Controller manager service to list controllers
    // controller_list_srv_ = node_->create_client<controller_manager_msgs::srv::ListControllers>("controller_manager/list_controllers");

    // controller_manager_srv_->wait_for_service();
    // controller_list_srv_->wait_for_service();

    // making sure the proper controller is running
    // ctrlRunCheck();

    // Controller manager service to manage the io controller
    io_client_srv_ = node_->create_client<ur_msgs::srv::SetIO>("/io_and_status_controller/set_io", rmw_qos_profile_services_default, io_cb_group_);
    io_client_srv_->wait_for_service();

    RCLCPP_INFO(node_->get_logger(), "Turning ON the vacuum generator");
    ioStartStop(1);

    ur3e_isReady = true;

  }

  void ioStartStop(unsigned short int valPin)
  {
    // changing tool voltage to 12V or 0V to enable laser pointer
    auto request_io = std::make_shared<ur_msgs::srv::SetIO::Request>();
    request_io->fun = 4;  // 4 is FUN_SET_TOOL_VOLTAGE
    request_io->pin = 17;   // Pin number, not important
    if (valPin == 1) {
      request_io->state = 12; // State
    }
    else {
      request_io->state = 0; // State
    }

    // send the request and wait for the result
    auto future_io = io_client_srv_->async_send_request(request_io);
    if (valPin == 1) {
      rclcpp::spin_until_future_complete(node_->get_node_base_interface(), future_io);
      auto result_io = future_io.get();
      if (result_io->success == true) {
        RCLCPP_INFO(node_->get_logger(), "Laser pointer is activated");
      }
      else {  
        RCLCPP_ERROR(node_->get_logger(), "IO Start/Stop failed");
      }
    }
    else if (valPin == 0) {
      rclcpp::sleep_for(std::chrono::seconds(1));
      RCLCPP_INFO(node_->get_logger(), "Laser pointer should be deactivated");
    }
    

    //turning ON the Vacuum generator
    // auto request_io = std::make_shared<ur_msgs::srv::SetIO::Request>();
    request_io->fun = 1;  // 1 is digital output
    request_io->pin = 12;   // Pin number
    request_io->state = valPin; // State

    // send the request and wait for the result
    future_io = io_client_srv_->async_send_request(request_io);
    if (valPin == 1) {
      rclcpp::spin_until_future_complete(node_->get_node_base_interface(), future_io);
      auto result_io = future_io.get();
      if (result_io->success == true) {
        RCLCPP_INFO(node_->get_logger(), "Vacuum generator is ON");
      }
      else {  
        RCLCPP_ERROR(node_->get_logger(), "IO Start/Stop failed");
      }
    }
    else if (valPin == 0) {
      rclcpp::sleep_for(std::chrono::seconds(1));
      RCLCPP_INFO(node_->get_logger(), "Vacuum generator should be off");
    }
  }

  void ioCtrl(unsigned short int numPin, unsigned short int valPin)
  {
    auto request_io = std::make_shared<ur_msgs::srv::SetIO::Request>();
    request_io->fun = 1;  // 1 is digital output
    request_io->pin = numPin;   // Pin number
    request_io->state = valPin; // State, as far as I know it can be also voltage

    // send the request and wait for the result
    auto future_io = io_client_srv_->async_send_request(request_io);
    std::future_status status_io = future_io.wait_for(4s);  // timeout to guarantee a graceful finish
    if (status_io == std::future_status::ready) {
      RCLCPP_INFO(node_->get_logger(), "IO service success");
    }
    else {
      RCLCPP_INFO(node_->get_logger(), "IO service fail");
    }
  }

private:
  rclcpp::Node::SharedPtr node_;

  rclcpp::CallbackGroup::SharedPtr client_cb_group_;
  rclcpp::CallbackGroup::SharedPtr sub_cb_group_;
  rclcpp::CallbackGroup::SharedPtr io_cb_group_;
  rclcpp::CallbackGroup::SharedPtr lp_cb_group_;

  rclcpp::Subscription<ur3e_mrc_msgs::msg::CommandUR3e>::SharedPtr sub_comm_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_lp_;  

  rclcpp_action::Client<control_msgs::action::FollowJointTrajectory>::SharedPtr client_traj_;

  rclcpp::Client<ur_msgs::srv::SetIO>::SharedPtr io_client_srv_;
  rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr controller_manager_srv_;
  rclcpp::Client<controller_manager_msgs::srv::ListControllers>::SharedPtr controller_list_srv_;  

  void ctrlRunCheck()
  {
    RCLCPP_INFO(node_->get_logger(), "Running scaled_joint_trajectory_controller check");

    auto request_list = std::make_shared<controller_manager_msgs::srv::ListControllers::Request>();
    auto future_list = controller_list_srv_->async_send_request(request_list);
    rclcpp::spin_until_future_complete(node_->get_node_base_interface(), future_list);

    auto result_list = future_list.get();
    for (auto& controller : result_list->controller) {
      // Check the list of controllers
      // RCLCPP_INFO(node_->get_logger(), "Available controller: %s", controller.name.c_str());      
      if (controller.name == CTRL_TO_RUN) {
        if (controller.state != "active") {
          RCLCPP_INFO(node_->get_logger(), "Attempting to activate scaled_joint_trajectory_controller");
          std::vector<std::string> ctrl_to_run;
          ctrl_to_run.push_back(CTRL_TO_RUN);

          auto request_sw = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
          request_sw->strictness = request_sw->STRICT;
          request_sw->activate_controllers = ctrl_to_run;
          
          auto future_sw = controller_manager_srv_->async_send_request(request_sw);
          rclcpp::spin_until_future_complete(node_->get_node_base_interface(), future_sw);

          auto result_sw = future_sw.get();
          if (result_sw->ok == false) {
            RCLCPP_ERROR(node_->get_logger(), "Could not activate scaled_joint_trajectory_controller");
          }
          else {
            RCLCPP_INFO(node_->get_logger(), "The scaled_joint_trajectory_controller was successfully activated");
          }
        }
      }
    }
  }

  bool sendTrajectory(control_msgs::action::FollowJointTrajectory::Goal goal)
  {
    goal.trajectory.header.stamp = node_->now();

    RCLCPP_INFO(node_->get_logger(), "Sending goal");
    auto goal_handle_future = client_traj_->async_send_goal(goal);

    // waiting for result of the call
    std::future_status send_goal_status = goal_handle_future.wait_for(10s);  // timeout to guarantee a graceful finish
    if (send_goal_status != std::future_status::ready) {
      RCLCPP_INFO(node_->get_logger(), "Send goal call failed");
      return false;
    }
    RCLCPP_INFO(node_->get_logger(), "Send goal call succeded");

    auto result_future = client_traj_->async_get_result(goal_handle_future.get());

    // Wait for the server to be done with the goal    
    result_future.wait_for(10s);  // timeout to guarantee a graceful finish
    if (result_future.get().code == rclcpp_action::ResultCode::SUCCEEDED) {
      RCLCPP_INFO(node_->get_logger(), "Goal was reached");
      return true;
    }
    RCLCPP_ERROR(node_->get_logger(), "Goal was never reached");
    return false;
  }

  void initGoal(control_msgs::action::FollowJointTrajectory::Goal &goal)
  {
    // First, the joint names, which apply to all waypoints
    goal.trajectory.joint_names.push_back("shoulder_pan_joint");
    goal.trajectory.joint_names.push_back("shoulder_lift_joint");
    goal.trajectory.joint_names.push_back("elbow_joint");
    goal.trajectory.joint_names.push_back("wrist_1_joint");
    goal.trajectory.joint_names.push_back("wrist_2_joint");
    goal.trajectory.joint_names.push_back("wrist_3_joint");
    goal.trajectory.points.resize(1);
    goal.trajectory.points[0].velocities.resize(N_JOINTS);
    goal.trajectory.points[0].velocities = {}; // Initialize with all zeros
    goal.trajectory.points[0].accelerations.resize(N_JOINTS);
    goal.trajectory.points[0].accelerations = {}; // Initialize with all zeros
  }

  void lp_callback(const std_msgs::msg::Bool & msg)
  {
    if (msg.data && !laserP_isON && ur3e_isReady)
    {
      ioCtrl(17, 1); // turning the laser pointer ON
      laserP_isON = true;
    }
    else if (!msg.data && laserP_isON)
    {
      ioCtrl(17, 0); // turning the laser pointer OFF
      laserP_isON = false;
    }
  }

  void comm_callback(const ur3e_mrc_msgs::msg::CommandUR3e & msg)
  {
    // RCLCPP_INFO(node_->get_logger(), "Got ur3e command");
    if (msg.destination.size() != N_JOINTS)
    {
      RCLCPP_INFO(node_->get_logger(), "WARNNING: In commCallback- received command size is not 6");
      return;
    }

    // initiate the goal variable
    control_msgs::action::FollowJointTrajectory::Goal goal;

    if (msg.io_0 && !grip_isON)
    {
      ioCtrl(1, 1); // turning the gripper ON
      grip_isON = true;
    }
    else if (!msg.io_0 && grip_isON)
    {
      ioCtrl(1, 0); // turning the gripper OFF
      grip_isON = false;
    }

    initGoal(goal);

    // fill the positions of the goal
    goal.trajectory.points[0].positions.resize(N_JOINTS);
    goal.trajectory.points[0].positions = msg.destination;
    goal.trajectory.points[0].positions[0] += boost::math::constants::pi<double>() / 2;
    goal.trajectory.points[0].positions[3] -= boost::math::constants::pi<double>() / 2;
    goal.trajectory.points[0].time_from_start = rclcpp::Duration::from_seconds(2.0);
    // pt.time_from_start = get_duration(data.destination, data.v)

    if (ur3e_isReady) {
      // turning off the laser pointer off before moving
      ioCtrl(17, 0); // turning the laser pointer OFF
      laserP_isON = false;      

      ur3e_isReady = false;
      bool traj_result = sendTrajectory(goal);

      if (traj_result) {
        RCLCPP_INFO(node_->get_logger(), "Ready for a new goal");
        ur3e_isReady = true;
      }
      else {
        RCLCPP_INFO(node_->get_logger(), "Something went wrong, restart the node");
      }
    }
    else {
      RCLCPP_INFO(node_->get_logger(), "Trajectory controller is not ready yet");
    }

  }

};
}   // namespace cb_group_ur3e



int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto control_node = std::make_shared<rclcpp::Node>("ur3e_mrc_enpm662_control");
  rclcpp::executors::MultiThreadedExecutor executor;
  
  RCLCPP_INFO(control_node->get_logger(), "Starting the control node");
  cb_group_ur3e::UR3eENPM662Control ur3e_ctrl(control_node);
  
  executor.add_node(control_node);  
  executor.spin();

  // RCLCPP_INFO(control_node->get_logger(), "Starting trajectory rebuplisher");
  // UR3eTrajControl ur3e_traj_ctrl(control_node);

  // RCLCPP_INFO(control_node->get_logger(), "Starting IO controller");
  // UR3eIOControl ur3e_io_ctrl(control_node);
  
  // shutting down procedures
  // rclcpp::sleep_for(std::chrono::seconds(5));
  RCLCPP_INFO(control_node->get_logger(), "Keyboard interrupt, shutting down");
  ur3e_ctrl.ioStartStop(0);

  rclcpp::shutdown();

  return 0;
}