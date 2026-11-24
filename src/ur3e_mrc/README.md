# Ppackage used to publish modified js and io topics in ENPM662 lab sections

main steps to run 
1. turn ON UR3e
2. run the official UR ROS2 driver
3. run the external control on UR3e pendant
4. run this package code: ros2 launch ur3e_mrc ur3e_enpm662.launch
5. run python code that publishes to ur3/command


## test command
ros2 topic pub --once /ur3e/command ur3e_mrc_msgs/msg/CommandUR3e "destination: [0, -1.57, -1.57, 0, 0, 0]
v: 1.0
a: 1.0
io_0: false" 


the gripper the code is based on sdurobotics ur_rtde gripper example