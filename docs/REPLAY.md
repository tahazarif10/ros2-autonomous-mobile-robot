# Rosbag replay regression

This v0.4 slice verifies that a deterministic recorded input stream can be
replayed through the real lifecycle control adapter with a stable result.

## Checked-in source fixture

The human-readable source fixture is:

`src/amr_control_adapter/test/fixtures/control_replay.json`

It defines:

- a straight path from 0 m to 2 m
- an ordered sequence of `/plan` and `/odom` events
- nanosecond event timestamps
- a fixed replay speed

The test converts this source fixture into a real **rosbag2 sqlite3** bag using
the ROS 2 Jazzy `rosbag2_py.SequentialWriter` API.

## Replay contract

The same generated bag is then opened with
`rosbag2_py.SequentialReader` and replayed through the live ROS 2 topics of
the C++ lifecycle adapter **twice**.

Each replay must independently demonstrate:

- normal motion is observed
- diagnostics report `none` while motion is valid
- the terminal pose produces `goal_reached`
- the final published command is zero

The canonical outcome dictionary from replay 1 must equal replay 2.

This is deliberately a behavioral repeatability assertion rather than an exact
wall-clock timing assertion.

## Run the replay regression

From a ROS 2 Jazzy / Ubuntu 24.04 workspace:

```bash
source /opt/ros/jazzy/setup.bash
colcon build --packages-select amr_control_adapter
source install/setup.bash
colcon test --packages-select amr_control_adapter --ctest-args -R test_rosbag_replay
colcon test-result --verbose
```

The test creates its sqlite3 bag in a temporary directory and removes it after
the run, so generated binary artifacts are not committed to the repository.

## Scope

This verifies deterministic control-boundary replay for the checked-in fixture.
It does not establish real-time determinism, physical robot repeatability, or
sensor/localization performance.


## Captured metrics

Each replay captures and verifies:

- bag message count
- source bag timestamp span
- paced replay wall duration within a deliberately broad CI-safe bound
- maximum absolute linear command
- command sample count
- diagnostic sample count

For the checked-in source fixture, the bag contains **8 messages** spanning
**1.2 s** of recorded time and is replayed at **2.0×** pacing. The test does
not require exact wall-clock equality between CI runs; it verifies the recorded
timing contract and bounded runtime behavior instead.

## Missing-TF fail-closed regression

The navigation fixture can be launched with:

`start_loopback:=false`

This intentionally removes the provider of `map -> odom` and
`odom -> base_link`. The system test verifies that:

- `map -> base_link` remains unavailable
- `bt_navigator` never reaches ACTIVE state during the observation window
- no non-zero `cmd_vel` is published

This is a software fail-closed check for a missing localization/odometry TF
contract. It is not a physical safety certification.
