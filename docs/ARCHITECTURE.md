# Architecture

## Design intent

This repository separates **robotics algorithms** from **ROS 2 transport and lifecycle concerns**.

The standalone `robotics-control-core` project owns reusable algorithmic code such as path shaping, tracking, kinematics, and odometry. This repository owns ROS 2 composition: messages, TF, lifecycle, QoS, launch, simulation, Nav2 integration, replay, and system-level verification.

That split is deliberate. It keeps the control math testable without ROS while making the ROS layer thin enough to review and reason about.

## Runtime data flow

1. Robot or simulator publishes state.
2. Robot-state components maintain fixed/joint transforms.
3. Odometry and localization provide the global robot pose.
4. Nav2 owns navigation task orchestration and global/local navigation behavior.
5. The standalone control-core remains available behind a lifecycle-aware ROS 2 adapter for reusable controller experiments and integration work.
6. Motion commands are bounded before they reach the simulated or physical drive boundary.
7. Tests verify lifecycle, stale/invalid input handling, TF ownership, and deterministic navigation behavior.

## Frame contract

The v0.3 deterministic fixture uses:

```text
map
└── odom                  nav2_loopback_sim
    └── base_link         nav2_loopback_sim
        ├── base_scan     robot_state_publisher
        ├── left_wheel_link
        ├── right_wheel_link
        └── caster_link
```

Ownership rules:

- the loopback localization/odometry fixture owns `map -> odom` and `odom -> base_link`
- `robot_state_publisher` owns fixed/joint transforms below `base_link`
- the system test verifies the required TF chain at runtime
- no second component is configured to publish `map -> odom` in the deterministic fixture

## Package boundaries

### amr_description

Owns robot geometry, kinematic dimensions, URDF/Xacro, and visualization transforms.

### amr_bringup

Owns launch composition, the v0.3 Nav2 loopback fixture, static maps, system configuration, and launch-level verification.

### amr_control_adapter

Owns the lifecycle-aware ROS 2 translation around `robotics-control-core`:

- Path/Odometry input conversion
- parameter validation
- lifecycle state
- stale-input handling
- bounded velocity command publication
- safe-stop behavior

It does not fork or copy the underlying control algorithms.

## QoS contract

The deterministic Nav2 fixture documents the transport assumptions used by its runtime components:

- `/initialpose`: reliable, volatile, depth 10 on the loopback subscriber; the test waits for discovery before publishing
- `/odom`: reliable, volatile, depth 10
- `/cmd_vel`: explicitly unstamped `geometry_msgs/msg/Twist` via `enable_stamped_cmd_vel: false`
- static map: transient-local subscription in both costmaps
- loopback `/scan`: best-effort, volatile sensor QoS

The exact fixture behavior and rationale are documented in `docs/NAV2_FIXTURE.md`.

## Safety and failure handling

The repository already tests:

- stale odometry/path in the control adapter
- non-finite input handling
- command saturation
- lifecycle transitions
- invalid runtime configuration
- deterministic Nav2 goal completion
- fixture-scoped obstacle clearance
- TF-chain availability
- navigator lifecycle readiness before goal submission

v0.4 extends this with rosbag replay, missing-TF injection, diagnostics, and timing/observability.

## Verification strategy

Verification is layered:

1. standalone algorithm tests in `robotics-control-core`
2. adapter unit tests for type conversion and parameter validation
3. ROS 2 launch/integration tests
4. deterministic Nav2 loopback fixture
5. rosbag replay regression
6. CI build/test gates
7. hardware validation only after simulation behavior is reproducible

Metrics are fixture-scoped and must not be presented as physical-hardware performance without separate hardware evidence.
