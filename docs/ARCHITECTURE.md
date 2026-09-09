# Architecture

## Design intent

This repository separates **robotics algorithms** from **ROS 2 transport and lifecycle concerns**.

The standalone `robotics-control-core` project owns reusable algorithmic code such as path shaping, tracking, kinematics, and odometry. This repository owns ROS 2 composition: messages, TF, lifecycle, QoS, launch, simulation, Nav2 integration, replay, and system-level verification.

That split is deliberate. It keeps the control math testable without ROS while making the ROS layer thin enough to review and reason about.

## Planned runtime data flow

1. Robot/simulator publishes joint and sensor state.
2. Robot-state components maintain the TF tree.
3. Odometry and localization provide the robot pose.
4. Nav2 owns navigation task orchestration and global/local navigation behavior.
5. A control adapter converts ROS 2 path/state inputs into the standalone control-core types.
6. The control core produces a bounded motion command.
7. The drive interface publishes the command to the simulated or physical differential-drive base.
8. Diagnostics expose readiness, stale data, lifecycle state, command saturation, and fault conditions.

## Frame contract

Initial frame contract:

```text
map
└── odom
    └── base_link
        ├── left_wheel_link
        ├── right_wheel_link
        └── caster_link
```

Ownership rules:

- localization owns `map -> odom`
- odometry owns `odom -> base_link`
- robot description owns fixed/joint transforms below `base_link`
- no component may publish a transform owned by another layer

## Package boundaries

### amr_description

Owns robot geometry, kinematic dimensions, URDF/Xacro, and visualization assets.

### amr_bringup

Owns launch composition and runtime configuration. It should contain as little algorithmic code as possible.

### amr_control_adapter — planned

Owns ROS 2 translation around `robotics-control-core`:

- subscriptions for path/state
- parameter validation
- lifecycle state
- QoS contracts
- stale-input handling
- command publication
- diagnostics

It must not fork/copy the underlying control algorithms.

### amr_localization — planned

Owns localization configuration and replayable state-estimation experiments. The exact estimator will be selected with measurable fixtures rather than by adding dependencies for appearance.

## Safety and failure handling

The runtime design will explicitly test:

- stale odometry
- stale path
- missing TF
- invalid/NaN input
- command saturation
- lifecycle transitions
- controller failure to converge
- shutdown while active

The safe output for invalid or stale motion inputs is a bounded stop command unless a more specific contract is documented and tested.

## Verification strategy

Verification is layered:

1. standalone algorithm tests in `robotics-control-core`
2. adapter unit tests for type conversion and parameter validation
3. ROS 2 launch/integration tests
4. deterministic simulation fixtures
5. rosbag replay regression
6. CI build/test gates
7. hardware validation only after simulation behavior is reproducible

Metrics will be fixture-scoped and may include goal error, cross-track error, collision status, command saturation, tracking time, and replay determinism.
