# Verification

This file records reproducible hosted evidence for completed project milestones.

## v0.1 — Reproducible ROS 2 baseline

Status: **complete**

Baseline:

- ROS 2 Jazzy
- Ubuntu 24.04
- GitHub Actions
- `colcon build`
- `colcon test`

Verified implementation:

- merge commit: `01c4ecab7047e4cf551a82ce79d0c35ff3f7d988`
- pull request: [#5 — test: verify ROS 2 baseline description and launch](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/5)
- hosted CI run: [#34379520128](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34379520128)
- hosted result: **success**

The hosted CI run completed successfully on the merged `main` commit and covered:

- dependency resolution with `rosdep`
- workspace build with `colcon build`
- Xacro expansion and robot-structure assertions
- joint/link integrity checks
- wheel-axis contract checks
- headless `amr_bringup` launch smoke test
- runtime discovery of `/robot_state_publisher`
- runtime discovery of `/joint_state_publisher`
- `colcon test-result --verbose`

## v0.1 evidence scope

This evidence verifies the checked-in ROS 2 baseline and automated tests only. It is not a hardware-performance claim.

## v0.2 — Lifecycle control-core adapter

Status: **complete**

Verified implementation:

- merge commit: `42c17d7829697a86cec8bc12a64ee66e25b84765`
- pull request: [#6 — feat: add lifecycle control-core adapter](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/6)
- hosted CI run on merged `main`: [#34380715133](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34380715133)
- hosted result: **success**
- pinned control-core dependency: `dda918bb362c5aa79f025a8f89664a215bb5a33c`

The hosted CI run verified:

- dependency resolution on ROS 2 Jazzy / Ubuntu 24.04
- compilation of the C++20 lifecycle adapter
- direct linking against the pinned `robotics::control` CMake target
- safe-stop behavior for missing, stale, and non-finite inputs
- bounded linear/angular command output
- goal-reached stop behavior
- nominal lifecycle transitions: configure → activate → deactivate → cleanup
- rejection of invalid runtime configuration
- the existing v0.1 Xacro and launch smoke tests

## v0.2 evidence scope

This verifies the checked-in software contract and hosted tests. It does not claim physical-robot behavior.

## v0.3 — Deterministic Nav2 integration

Status: **complete**

Verified implementation:

- primary merge commit: `62fede538f84671355ef330d04c6aea1fdd2e629`
- primary pull request: [#8 — feat: add deterministic Nav2 loopback fixture](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/8)
- primary merged-main CI run: [#34384788427](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34384788427)
- TF/QoS verification merge commit: `2e3f673b4556495ea5ad3824252c6f0f94e1b92e`
- TF/QoS pull request: [#9 — test: verify v0.3 TF and QoS contract](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/9)
- PR #9 hosted CI run: [#34385774324](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34385774324)
- final merged-main CI run: [#34386081006](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34386081006)
- hosted result: **success**

The v0.3 fixture verifies:

- repository-owned Nav2 launch and configuration
- static-map loading from the checked-in 6 m × 6 m fixture
- NavFn global planning with A* enabled
- Regulated Pure Pursuit control
- loopback-provided deterministic localization/odometry
- runtime availability of `map <- odom <- base_link <- base_scan`
- documented and compatible QoS assumptions for initial pose, odometry, velocity commands, static map, and loopback scan
- lifecycle activation of `bt_navigator` before a navigation goal is sent
- successful `NavigateToPose` completion
- collision clearance against the checked-in central obstacle expanded by robot radius
- final position error within 0.20 m for the fixture
- non-trivial detour around an obstacle that blocks the direct start-to-goal path

## v0.3 evidence scope

This is deterministic software/system-integration evidence for the exact checked-in loopback fixture. It is not evidence of physical dynamics, localization accuracy under sensor noise, real-world collision avoidance, or hardware performance.
