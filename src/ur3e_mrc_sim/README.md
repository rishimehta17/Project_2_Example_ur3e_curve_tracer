# Ppackage used to publish modified js topics in ENPM662 Gazebo simulations

main steps to run 
1. run the Gazebo simulation for ENPM662
2. run this package code: ros2 launch ur3e_mrc_sim ur3e_enpm662.launch
5. run python code that publishes to ur3e/command


## test command
ros2 topic pub --once /ur3e/command ur3e_mrc_msgs/msg/CommandUR3e "destination: [0, -1.57, -1.57, 0, 0, 0]
v: 1.0
a: 1.0
io_0: false" 
