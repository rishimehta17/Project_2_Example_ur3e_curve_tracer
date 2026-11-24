#!/usr/bin/env python3
"""Live plot of the UR3e tool path, seen from above.

Every 50 ms it asks TF where the tool (tool0) is in the frame `base`, and draws the x-y path.
It only watches TF, so it shows what the robot really does, whatever program moves it.
Close the window or press Ctrl+C to stop; the plot is saved as live_plot.png.

Run:
    ros2 run ur3e_curve_tracer live_plot --ros-args -p use_sim_time:=true
"""

import threading

import matplotlib.pyplot as plt
import rclpy
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node
from rclpy.signals import SignalHandlerOptions
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener


def main():
    # Let Ctrl+C reach our drawing loop (as KeyboardInterrupt), so we can save and exit cleanly.
    rclpy.init(signal_handler_options=SignalHandlerOptions.NO)
    node = Node('live_plot')

    # TF: the listener receives the robot's frames and stores them in the buffer.
    tf = Buffer()
    listener = TransformListener(tf, node)  # noqa: F841 (must stay alive to keep TF updating)
    xs, ys = [], []                         # the tool path, shared by both threads

    def sample():
        """Called 20 times per second: store the current tool position."""
        try:
            # Time() = the latest available transform.
            p = tf.lookup_transform('base', 'tool0', Time()).transform.translation
        except Exception:
            return                          # TF not ready yet: skip this sample
        xs.append(p.x)
        ys.append(p.y)

    node.create_timer(0.05, sample)         # 20 Hz

    # Run ROS in a background thread, so sampling keeps up while the window redraws.
    executor = SingleThreadedExecutor()
    executor.add_node(node)
    ros = threading.Thread(target=executor.spin)
    ros.start()

    # The plot window.
    line, = plt.plot([], [])
    plt.axis('equal')                       # same scale on x and y: no distortion
    plt.xlabel('x [m]')
    plt.ylabel('y [m]')
    plt.title('Tool path')

    # Redraw 10 times per second until the window is closed or Ctrl+C is pressed.
    try:
        while plt.fignum_exists(line.figure.number):
            line.set_data(xs, ys)
            plt.gca().relim()               # fit the axes to the new points
            plt.gca().autoscale_view()
            plt.pause(0.1)
    except KeyboardInterrupt:
        pass

    # Save, then stop in order: ROS thread first, then ROS itself.
    line.figure.savefig('live_plot.png')
    executor.shutdown()                     # makes executor.spin() return
    ros.join()                              # wait for the ROS thread to end
    rclpy.shutdown()


if __name__ == '__main__':
    main()
