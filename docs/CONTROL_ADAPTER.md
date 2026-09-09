# Control adapter contract

The `amr_control_adapter` package is the ROS 2 boundary around
[`robotics-control-core`](https://github.com/tahazarif10/robotics-control-core).

The dependency is fetched at the exact commit:

`dda918bb362c5aa79f025a8f89664a215bb5a33c`

No planner/controller implementation is copied into this repository.

## Inputs

### `plan` — `nav_msgs/msg/Path`

QoS:

- depth: 1
- reliable
- transient local

The adapter uses the XY positions from the path. A new path resets the pure-pursuit
target state.

### `odom` — `nav_msgs/msg/Odometry`

QoS: ROS 2 sensor-data profile.

The adapter converts position and quaternion yaw to the middleware-independent
`robotics_control::Pose2D` contract.

## Output

### `cmd_vel` — `geometry_msgs/msg/Twist`

The lifecycle publisher is active only in the node's active state.

Commands are clamped to the configured linear and angular limits even though the
underlying controller already has limits. The adapter boundary therefore does not
trust upstream output implicitly.

## Safe-stop contract

The output command is zero when any of these conditions is true:

- no path is available
- odometry is stale
- path is stale
- pose/path data contains non-finite values
- input age is invalid
- the controller reports the goal reached
- the node is active but required runtime state has not arrived

On deactivation, the node publishes a zero command before deactivating the lifecycle
publisher.

## Parameters

| Parameter | Default | Contract |
| --- | ---: | --- |
| `control_rate_hz` | 20.0 | > 0 and finite |
| `stale_timeout_s` | 0.5 | > 0 and finite |
| `lookahead_m` | 0.55 | > 0 and finite |
| `max_linear_mps` | 0.85 | > 0 and finite |
| `max_angular_rps` | 1.8 | > 0 and finite |
| `goal_tolerance_m` | 0.10 | > 0 and finite |
| `curvature_slowdown` | 0.70 | >= 0 and finite |

## Lifecycle

The package implements a ROS 2 lifecycle node:

- configure: validate parameters and create subscriptions/publisher
- activate: activate `cmd_vel` and start the control timer
- deactivate: stop timer, publish zero, deactivate publisher
- cleanup: clear runtime state and release ROS interfaces
- shutdown: stop timer and clear runtime state

## Verification scope

Current tests exercise the middleware-independent safety policy, the exact
`robotics-control-core` dependency, nominal lifecycle transitions, and rejection of
invalid runtime configuration. Hosted ROS 2 CI is required before this milestone is
considered complete.
