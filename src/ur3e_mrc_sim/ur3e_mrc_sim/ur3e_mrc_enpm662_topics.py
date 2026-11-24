#!/usr/bin/env python3

import math
import rclpy
from rclpy.node import Node

from std_msgs.msg import Bool
from sensor_msgs.msg import JointState
from ur3e_mrc_msgs.msg import PositionUR3e
from ur3e_mrc_msgs.msg import GripperInput

class UR3eMRC_topics(Node):
    """Node for publishing modified joint states and io states topics."""

    def __init__(self):
        super().__init__("ur3e_mrc_enpm662_js_io")

        # subscribing to the UR3e joint states topic from the UR driver
        self.sub_js_ = self.create_subscription(JointState, "joint_states", self.js_callback, 10)
        self.sub_js_  # prevent unused variable warning
        self.get_logger().info(f"Subscribed to joint states")

        # publishing modified joint states values
        self.pub_pos_ = self.create_publisher(PositionUR3e, "ur3e/position", 10)

        # # publishing gripper related topics
        # self.pub_grasp_ = self.create_publisher(Bool, "gripper/grasping", 10)
        # self.pub_grip_ = self.create_publisher(GripperInput, "ur3e/gripper_input", 10)

    # callback for joint states subscriber
    def js_callback(self, msg):
        pos_msg = PositionUR3e()

        # pos_msg.position = [msg.position[5] - math.pi/2, msg.position[0], msg.position[1], msg.position[2] + math.pi/2, msg.position[3], msg.position[4]]
        pos_msg.position = [msg.position[0] - math.pi/2, msg.position[1], msg.position[2], msg.position[3] + math.pi/2, msg.position[4], msg.position[5]]
        pos_msg.is_ready = True

        self.pub_pos_.publish(pos_msg)


def main(args=None):
    rclpy.init(args=args)

    ur3e_mrc_enpm662_js_io = UR3eMRC_topics()
    try:
        rclpy.spin(ur3e_mrc_enpm662_js_io)
    except RuntimeError as err:
        ur3e_mrc_enpm662_js_io.get_logger().error(str(err))
    except SystemExit:
        rclpy.logging.get_logger("ur3e_mrc_enpm662_js_io").info(f"JS IO node exited")

    rclpy.shutdown()


if __name__ == "__main__":
    main()