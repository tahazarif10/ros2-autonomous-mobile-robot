# Control observability and fault injection

This document records the first v0.4 slice. It adds runtime observability and
deterministic fault injection around the lifecycle control adapter. It does not
yet claim that the full v0.4 replay milestone is complete.

## Diagnostics topic

The active control adapter publishes `diagnostic_msgs/msg/DiagnosticArray` on:

`diagnostics`

Each control tick exposes:

- `motion_enabled`
- `stop_reason`
- `control_sequence`
- `odometry_age_s`
- `path_age_s`

The diagnostic status message is the stable machine-readable stop reason:

- `none`
- `missing_path`
- `missing_odometry`
- `stale_path`
- `stale_odometry`
- `invalid_input`
- `goal_reached`

Severity is scoped to the software control boundary:

- OK: normal motion or goal reached
- WARN: missing/stale runtime input
- ERROR: invalid/non-finite runtime input

## Fault-injection regression

The hosted integration test drives the real lifecycle node through ROS 2 topics
rather than calling policy functions directly.

It verifies this sequence:

1. publish a valid path and odometry and observe non-zero bounded motion
2. inject NaN odometry and observe a zero command plus `invalid_input`
3. restore valid inputs, stop updating odometry, and observe a zero command plus
   `stale_odometry`
4. restore valid inputs, keep odometry fresh while allowing the path to age, and
   observe a zero command plus `stale_path`

This test checks both the command-side safety behavior and the diagnostic reason
reported to operators/tests.

## Rosbag replay

The second v0.4 slice adds a real rosbag2 sqlite3 replay regression. See
[`REPLAY.md`](REPLAY.md) for the checked-in source fixture, generation method,
repeatability contract, and reproduction command.

## Remaining v0.4 work

The following remain intentionally open:

- missing-TF fault injection in the navigation layer
- replay timing/latency metrics
- verification record on merged `main`

No physical-hardware reliability claim is made from these software tests.
