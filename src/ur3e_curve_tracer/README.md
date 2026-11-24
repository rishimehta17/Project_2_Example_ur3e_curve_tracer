# ur3e_curve_tracer

ROS 2 package for the ENPM662 UR3e environment. Give an equation `x(t), y(t)`, and the UR3e draws it
(tool pointing down, fixed height). It works in Gazebo and on the real robot.

| Command | What it does |
|---|---|
| `ros2 run ur3e_curve_tracer trace_curve` | Equation → points → IK → one joint trajectory → `/scaled_joint_trajectory_controller/follow_joint_trajectory`. Parameters: `x`, `y` (text, `t` from 0 to 2π, values in [−1, 1]). |
| `ros2 run ur3e_curve_tracer live_plot` | Live plot (from above) of where TF says the tool (`tool0`) is in the frame `base`. Saves `live_plot.png` on exit. |

In simulation add `--ros-args -p use_sim_time:=true` to both. On the real robot, don't.

Full instructions (Docker setup, simulation, real robot, safety checklist) are in the
[repository README](../../README.md).
