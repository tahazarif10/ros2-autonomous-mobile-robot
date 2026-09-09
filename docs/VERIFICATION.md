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


## v0.4 slice 1 — Control observability and fault injection

Status: **slice complete; v0.4 milestone remains open**

Verified implementation:

- merge commit: `65824d8047e241482b830fc11e55707a77b4a6f1`
- pull request: [#10 — feat: add control diagnostics and fault injection](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/10)
- PR hosted CI run: [#34388062539](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34388062539)
- merged-main CI run: [#34388363553](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34388363553)
- hosted result: **success**

The hosted integration test verifies the real lifecycle node through ROS 2 topics:

- valid path + odometry produces bounded motion and `stop_reason=none`
- injected NaN odometry produces a zero command and `stop_reason=invalid_input`
- stale odometry produces a zero command and `stop_reason=stale_odometry`
- stale path while odometry remains fresh produces a zero command and `stop_reason=stale_path`
- diagnostics expose motion state, stable stop reason, control sequence, odometry age, and path age
- missing path and missing odometry have distinct diagnostic reasons

## v0.4 slice 1 evidence scope

This verifies software-level failure handling and observability at the lifecycle control boundary. The full v0.4 milestone remains open until rosbag replay, replay repeatability, missing-TF navigation injection, and replay/timing evidence are implemented.


## v0.4 — Replay, observability, and fault injection

Status: **complete**

### Slice 1 — control diagnostics and input fault injection

- merge commit: `65824d8047e241482b830fc11e55707a77b4a6f1`
- PR: [#10](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/10)
- merged-main CI: [#34388363553](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34388363553) — **success**

Verified: stable diagnostic stop reasons, input ages, NaN/non-finite input handling,
stale odometry, stale path, and zero-command safe stop.

### Slice 2 — deterministic rosbag2 replay

- merge commit: `8cce3eefdc105eb1917a3adad2b507dcbf498935`
- PR: [#11](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/11)
- merged-main CI: [#34389798773](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34389798773) — **success**

The test generates a real sqlite3 rosbag using the ROS 2 Jazzy
`rosbag2_py.SequentialWriter`, then replays the same bag twice through the live
C++ lifecycle adapter using `SequentialReader`. Both runs must produce the same
canonical outcome: normal motion observed, goal reached, and final zero command.

### Slice 3 — missing-TF fail-closed behavior and replay metrics

- merge commit: `6942d60e7e31be21f0914fac73c0e319149c62cc`
- PR: [#12](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/12)
- PR CI: [#34394204856](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34394204856) — **success**

Verified:

- loopback localization/odometry can be intentionally disabled
- `map -> base_link` remains unavailable when the required TF provider is removed
- `bt_navigator` never reaches ACTIVE state in the observation window
- no non-zero `cmd_vel` is published
- replay fixture contains 8 messages spanning 1.2 s of recorded time at 2.0× configured pacing
- replay captures wall duration, maximum linear command, command sample count, and diagnostic sample count with CI-safe bounded assertions

### CI isolation fix discovered by merged-main verification

The first main-branch run after #12 exposed cross-test graph contamination rather
than an algorithm failure: package-level parallel test execution allowed a replay
`/odom` sample to be observed by the Nav2 trajectory checker.

The fix was implemented without relaxing the collision assertion:

- stabilization merge commit: `8d41bb559a5e85cd8d1ed329c6599529e8fdfe4f`
- PR: [#13](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/pull/13)
- PR CI: [#34395114124](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34395114124) — **success**
- final merged-main CI: [#34395404807](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/runs/34395404807) — **success**

The control replay and fault-injection fixtures now use isolated ROS namespaces,
while Nav2 launch fixtures are serialized inside `amr_bringup`.

## v0.4 evidence scope

This is software/system-integration evidence for checked-in deterministic fixtures.
It does not establish physical safety certification, hard real-time guarantees,
physical localization accuracy, or real-robot repeatability.
