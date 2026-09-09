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

## Scope of evidence

This evidence verifies the checked-in ROS 2 baseline and automated tests only. It is not a hardware-performance claim and does not yet demonstrate autonomous navigation, localization, or physical-robot operation.


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

This verifies the checked-in software contract and hosted tests. It does not yet claim
Nav2 integration, localization performance, simulator navigation performance, or
physical-robot behavior.
