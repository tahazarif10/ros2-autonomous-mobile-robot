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

## QoS contract

The fixture makes the transport assumptions explicit so the system test is not
accidentally dependent on an implicit ROS 2 default:

- `/initialpose` uses the loopback simulator's depth-10 reliable, volatile
  subscription contract. The test waits for discovery before publishing the
  initial pose that establishes the global transform.
- `/odom` is published by the loopback simulator with a depth-10 reliable,
  volatile QoS profile and is consumed as deterministic state feedback by the
  fixture test.
- `/cmd_vel` is explicitly configured as **unstamped**
  `geometry_msgs/msg/Twist` with `enable_stamped_cmd_vel: false` so the
  simulator and Nav2 command path agree on message type.
- The local and global costmaps configure
  `map_subscribe_transient_local: true`, matching the latched/static-map
  delivery contract.
- The loopback `/scan` publisher uses best-effort, volatile sensor QoS.
  Navigation correctness in this v0.3 fixture does not depend on scan data:
  both costmaps use the checked-in static map plus inflation.

## Runtime TF verification

The automated system test now verifies that all required transforms are
available at runtime after `/initialpose` is established:

```text
map <- odom
odom <- base_link
base_link <- base_scan
```

This complements the launch-level ownership contract: the loopback simulator
owns the two dynamic global/odometry transforms, while
`robot_state_publisher` owns the fixed `base_link -> base_scan` transform.
