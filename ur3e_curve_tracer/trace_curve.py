#!/usr/bin/env python3
"""Draw a curve x(t), y(t) with the UR3e.

Pipeline:
    equation  ->  200 points  ->  200 goal poses  ->  IK for each  ->  ONE joint trajectory
The trajectory is sent to the robot's trajectory controller, which plays it.
The same code works in Gazebo and on the real robot (same controller name).

Run:
    ros2 run ur3e_curve_tracer trace_curve --ros-args -p use_sim_time:=true
    ros2 run ur3e_curve_tracer trace_curve --ros-args -p use_sim_time:=true -p x:="cos(t)" -p y:="sin(t)"
"""

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from builtin_interfaces.msg import Duration
from control_msgs.action import FollowJointTrajectory
from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint

# Joint names, in the order the angles are listed in every trajectory point.
JOINTS = ['shoulder_pan_joint', 'shoulder_lift_joint', 'elbow_joint',
          'wrist_1_joint', 'wrist_2_joint', 'wrist_3_joint']

# Start pose [rad]: forearm level, tool pointing straight down.
# The tool is then at about (-0.299, -0.131, 0.303) m. This is the centre of the curve.
HOME = [0.0, -1.57, 1.57, -1.57, -1.57, 0.0]

# Half-width of the curve [m]: x and y in [-1, 1] become +-8 cm.
SIZE = 0.08

# UR3e DH parameters (standard DH), one value per joint, joint 1 first.
# They describe the frame `base` -> `tool0`. Note: ROS also has `base_link`,
# which is `base` turned 180 deg about z.
D = [0.15185, 0, 0, 0.13105, 0.08535, 0.0921]        # d_i     [m]
A = [0, -0.24355, -0.2132, 0, 0, 0]                  # a_i     [m]
ALPHA = [np.pi / 2, 0, 0, np.pi / 2, -np.pi / 2, 0]  # alpha_i [rad]


def fk(q):
    """Forward kinematics: 6 joint angles -> 4x4 pose of the tool (tool0) in base.

    The pose is a 4x4 matrix: rotation in T[:3, :3], position in T[:3, 3].
    """
    T = np.eye(4)
    for qi, d, a, al in zip(q, D, A, ALPHA):
        ct, st, ca, sa = np.cos(qi), np.sin(qi), np.cos(al), np.sin(al)
        # Standard DH transform of one joint: Rot_z(theta) Trans_z(d) Trans_x(a) Rot_x(alpha)
        T = T @ np.array([[ct, -st * ca, st * sa, a * ct],
                          [st, ct * ca, -ct * sa, a * st],
                          [0, sa, ca, d],
                          [0, 0, 0, 1]])
    return T


def error(T, T_goal):
    """How far pose T is from pose T_goal, as a 6-vector [position (3), rotation (3)].

    Rotation error: half the sum of the cross products of matching axes. It is a small
    rotation vector that turns T towards T_goal (good enough for small errors).
    """
    dp = T_goal[:3, 3] - T[:3, 3]
    dr = 0.5 * sum(np.cross(T[:3, i], T_goal[:3, i]) for i in range(3))
    return np.concatenate([dp, dr])


def ik(T_goal, q):
    """Inverse kinematics: find joint angles that put the tool at pose T_goal.

    Method: damped least squares, starting from q (the previous point's answer).
    Starting from the previous answer keeps the arm in the same configuration,
    so it never suddenly flips between neighbouring points.
    """
    q = np.array(q, dtype=float)
    for _ in range(100):                        # at most 100 steps (usually 2)
        e = error(fk(q), T_goal)
        if np.linalg.norm(e) < 1e-6:            # close enough: done
            break
        # Numerical Jacobian: move each joint a tiny bit and see how the pose changes.
        # Column j = change of the pose per radian of joint j.
        J = np.zeros((6, 6))
        for j in range(6):
            dq = np.zeros(6)
            dq[j] = 1e-6
            J[:, j] = error(fk(q), fk(q + dq)) / 1e-6
        # Damped least squares step: dq = J^T (J J^T + lambda^2 I)^-1 e, lambda = 0.01.
        # The damping keeps the step small near singularities (where J^-1 blows up).
        q += J.T @ np.linalg.solve(J @ J.T + 1e-4 * np.eye(6), e)
    return q


def point(q, t):
    """One trajectory point: joint angles q, to be reached t seconds after the start."""
    return JointTrajectoryPoint(positions=list(q),
                                time_from_start=Duration(sec=int(t), nanosec=int(t % 1 * 1e9)))


def main():
    rclpy.init()
    node = Node('trace_curve')

    # 1. The equation, as text, with t from 0 to 2*pi. Keep x and y in [-1, 1].
    #    Default: infinity sign, written so that t = 0 is its centre (= HOME), so no jump.
    x_expr = node.declare_parameter('x', 'sin(t) / (1 + cos(t)**2)').value
    y_expr = node.declare_parameter('y', 'sin(t) * cos(t) / (1 + cos(t)**2)').value

    # 2. Equation -> 200 points. eval() runs the text with all NumPy functions
    #    (sin, cos, sqrt, pi, ...) and t as an array. ones_like makes constants work too.
    t = np.linspace(0, 2 * np.pi, 200)
    xs = eval(x_expr, {**vars(np), 't': t}) * np.ones_like(t)
    ys = eval(y_expr, {**vars(np), 't': t}) * np.ones_like(t)

    # 3. Points -> one joint trajectory.
    traj = JointTrajectory(joint_names=JOINTS)
    traj.points.append(point(HOME, 4.0))        # first go to HOME, in 4 s
    T_home, q = fk(HOME), HOME
    for i, (x, y) in enumerate(zip(xs, ys)):
        # Goal pose = HOME pose shifted in x and y only.
        # So the height and the tool orientation (pointing down) stay fixed.
        T = T_home.copy()
        T[0, 3] += SIZE * x
        T[1, 3] += SIZE * y
        q = ik(T, q)                            # start from the previous answer
        traj.points.append(point(q, 5.0 + i * 0.1))   # one point every 0.1 s

    # 4. Send the whole trajectory to the controller (as one action goal) and wait.
    client = ActionClient(node, FollowJointTrajectory,
                          '/scaled_joint_trajectory_controller/follow_joint_trajectory')
    client.wait_for_server()                    # waits until Gazebo / the driver is up
    node.get_logger().info('Sending trajectory...')
    goal = client.send_goal_async(FollowJointTrajectory.Goal(trajectory=traj))
    rclpy.spin_until_future_complete(node, goal)          # wait: goal accepted
    result = goal.result().get_result_async()
    rclpy.spin_until_future_complete(node, result)        # wait: motion finished
    node.get_logger().info(f'Done, error code {result.result().result.error_code} (0 = OK)')
    rclpy.shutdown()


if __name__ == '__main__':
    main()
