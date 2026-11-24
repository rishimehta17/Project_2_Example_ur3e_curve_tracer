from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='ur3e_mrc_sim',
            executable='ur3e_mrc_enpm662_topics'
        )
    ])