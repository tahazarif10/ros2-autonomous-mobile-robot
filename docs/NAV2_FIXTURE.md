# Nav2 deterministic fixture

## Purpose

The v0.3 fixture verifies ROS 2 / Nav2 integration independently of a physics
simulator and independently of localization noise.

It uses Nav2's loopback simulator to provide a deterministic motion loop:

- `map -> odom` and `odom -> base_link` transforms
- odometry
- command-velocity consumption
- a synthetic scan frame
- relocation through `/initialpose`

The fixture deliberately does **not** claim physical robot performance.

## Scenario

- map: 6 m x 6 m at 0.05 m/cell
- start: (-2.0, 0.0)
- goal: (2.0, 0.0)
- central obstacle blocks the direct path
- robot radius: 0.20 m
- inflation radius: 0.45 m
- global planner: NavFn with A* enabled
- controller: Regulated Pure Pursuit
- localization source: Nav2 loopback simulator

The test passes only if Nav2 reports success, the recorded odometry trajectory
does not enter the obstacle expanded by robot radius, the final position is
within 0.20 m of the goal, and the trajectory demonstrates a real detour around
the obstacle.

## TF ownership in this fixture

```text
map
└── odom                  nav2_loopback_sim
    └── base_link         nav2_loopback_sim
        └── base_scan     robot_state_publisher (fixed URDF joint)
```

The fixture intentionally avoids AMCL so there is exactly one owner of
`map -> odom`.

## Reproduce

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch amr_bringup navigation_loopback.launch.py
```

The automated system test is run through `colcon test` in CI.
