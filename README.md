# UR3e Curve Tracer

Give an equation `x(t), y(t)`. The UR3e draws it in Gazebo: by default an infinity sign, at a
fixed height, with the tool pointing down.

## 1. Install Docker (once, Ubuntu 22.04)

```bash
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker $USER      # then log out and back in
docker run --rm hello-world        # check: works without sudo
```

## 2. Build the course image (once, ~7 GB)

```bash
git clone https://github.com/MarylandRoboticsCenter/ENME480_mrc.git ~/ENME480_mrc
cd ~/ENME480_mrc/docker
userid=$(id -u) groupid=$(id -g) docker compose -f humble-enme480_ur3e-compose.yml build
```

## 3. Get this code

```bash
cd ~/ENME480_mrc/src
git clone https://github.com/rishimehta17/Project_2_Example_ur3e_curve_tracer.git ur3e_curve_tracer
```

## 4. Start the container

```bash
xhost +local:docker
cd ~/ENME480_mrc/docker
docker compose -f humble-enme480_ur3e-compose.yml up -d
```

Open a terminal in it (repeat for each new terminal):

```bash
docker exec -it humble-enme480_ur3e-container bash
```

Build once, inside the container:

```bash
cd ~/enme480_ws && colcon build --symlink-install && source install/setup.bash
```

## 5. Run (3 terminals in the container)

In each terminal, first run `cd ~/enme480_ws && source install/setup.bash`.

```bash
# Terminal 1: Gazebo (wait until the arm appears)
ros2 launch enme480_gazebo enme480_ur3e_cubes.launch.py

# Terminal 2: live plot
ros2 run ur3e_curve_tracer live_plot --ros-args -p use_sim_time:=true

# Terminal 3: draw the curve
ros2 run ur3e_curve_tracer trace_curve --ros-args -p use_sim_time:=true
```

On the first run, the plot also shows the arm moving from Gazebo's start pose to HOME.

Another curve (keep x and y between -1 and 1):

```bash
ros2 run ur3e_curve_tracer trace_curve --ros-args -p use_sim_time:=true -p x:="cos(t)" -p y:="sin(t)"
```

Stop the container when done:

```bash
cd ~/ENME480_mrc/docker && docker compose -f humble-enme480_ur3e-compose.yml down
```

## Code

- `trace_curve.py`: the equation becomes 200 points. Each point gets the HOME pose shifted in x
  and y, IK turns it into joint angles, and all of them go to the robot as one trajectory.
- `live_plot.py`: plots the tool position (TF `base → tool0`) live. It saves `live_plot.png`
  when you close it.
