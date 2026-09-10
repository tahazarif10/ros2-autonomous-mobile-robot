# ROS 2 Autonomous Mobile Robot

[![CI](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/workflows/ci.yml/badge.svg)](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/workflows/ci.yml)
[![ROS 2](https://img.shields.io/badge/ROS%202-Jazzy-22314E)](https://docs.ros.org/en/jazzy/)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

[**Portfolio**](https://github.com/tahazarif10) · [**Resume**](https://github.com/tahazarif10/tahazarif10/blob/main/RESUME.md) · [**Evidence**](https://github.com/tahazarif10/tahazarif10/blob/main/EVIDENCE.md) · [**LinkedIn**](https://www.linkedin.com/in/taha-zarif-bba94b397/) · [**Control core**](https://github.com/tahazarif10/robotics-control-core)

**An engineering-oriented autonomous mobile robot stack built around ROS 2, C++, Nav2, deterministic testing, and reusable control software.**

The project is developed in measurable slices. Each milestone must leave behind buildable code, automated verification, and reproducible evidence rather than a demo-only repository.

## Why this repository exists

This project is the middleware/integration layer of the robotics portfolio. It consumes the standalone [`robotics-control-core`](https://github.com/tahazarif10/robotics-control-core) at a pinned commit rather than duplicating planning and control algorithms inside ROS 2 callbacks. That keeps algorithm behavior independently testable while this repository concentrates on lifecycle, TF, QoS, Nav2, replay, diagnostics, and integration failure modes.

## Baseline

- ROS 2 **Jazzy Jalisco**
- Ubuntu **24.04**
- C++ for runtime components
- Python for launch/test tooling
- `colcon` + `ament_cmake`
- GitHub Actions CI
- Apache-2.0

## Architecture

```mermaid
flowchart LR
    S[Sensors / Simulation] --> TF[TF2 + Robot State]
    S --> O[Odometry]
    O --> L[Localization]
    TF --> L
    L --> N[Nav2]
    N --> A[Control Adapter]
    A --> C[robotics-control-core]
    C --> V[cmd_vel / Drive Interface]
    V --> B[Differential Drive Base]
```

The lifecycle-aware control adapter consumes the existing [`robotics-control-core`](https://github.com/tahazarif10/robotics-control-core) library at a pinned commit rather than duplicating planner/controller math inside ROS 2 nodes.

## Current status

**v0.4 replay, observability, and fault injection — complete**

Verified software/system scope:

- lifecycle-aware C++20 control adapter
- bounded `cmd_vel` output and explicit safe-stop behavior
- deterministic Nav2 fixture using NavFn A* + Regulated Pure Pursuit
- explicit TF ownership and runtime TF verification
- documented QoS contracts
- diagnostics with stable stop reasons and input-age observability
- NaN, stale-odometry, and stale-path fault injection with zero-command safe stop
- real rosbag2 sqlite3 fixture generated from checked-in source data
- same bag replayed twice through the live lifecycle adapter with equal canonical outcomes
- missing-global-TF regression verifying `bt_navigator` remains non-ACTIVE and no non-zero `cmd_vel` is produced
- namespace/test-graph isolation to prevent concurrent ROS test contamination

The checked-in v0.3 navigation scenario is a 6 m × 6 m software fixture using NavFn with A* enabled and Regulated Pure Pursuit. Navigation, timing, replay, and safety-response results are explicitly scoped to the checked-in software fixtures and are not physical-hardware or safety-certification claims.

Hosted verification is recorded in [`docs/VERIFICATION.md`](docs/VERIFICATION.md). Replay and failure-handling contracts are documented in [`docs/REPLAY.md`](docs/REPLAY.md) and [`docs/OBSERVABILITY.md`](docs/OBSERVABILITY.md).

## Repository layout

```text
.
├── src/
│   ├── amr_description/      # Robot model and geometry
│   ├── amr_bringup/          # Launch, Nav2 fixture, maps, system composition
│   └── amr_control_adapter/  # Lifecycle ROS 2 boundary around control core
├── docs/
│   ├── ARCHITECTURE.md
│   ├── CONTROL_ADAPTER.md
│   ├── NAV2_FIXTURE.md
│   ├── OBSERVABILITY.md
│   ├── REPLAY.md
│   ├── VERIFICATION.md
│   └── ROADMAP.md
└── .github/workflows/
    └── ci.yml
```

## Build

On Ubuntu 24.04 with ROS 2 Jazzy installed:

```bash
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

Display the robot model:

```bash
ros2 launch amr_bringup display.launch.py
```

Run the deterministic Nav2 fixture manually:

```bash
ros2 launch amr_bringup navigation_loopback.launch.py
```

The end-to-end fixture assertion runs through `colcon test` in CI.

## Engineering goals

- clean TF tree and explicit frame ownership
- deterministic path/control regression tests
- lifecycle-aware ROS 2 nodes
- deliberate QoS choices rather than defaults by accident
- Nav2 integration without reimplementing Nav2
- reuse of standalone control algorithms through a thin ROS 2 adapter
- rosbag-based replay for reproducible regression testing
- simulation before hardware claims
- CI evidence for every merge

See [Architecture](docs/ARCHITECTURE.md), [Control adapter contract](docs/CONTROL_ADAPTER.md), [Nav2 fixture](docs/NAV2_FIXTURE.md), [Verification](docs/VERIFICATION.md), and [Roadmap](docs/ROADMAP.md).

## Related portfolio work

- [robotics-control-core](https://github.com/tahazarif10/robotics-control-core) — middleware-independent C++20 navigation/control core consumed by this stack.
- [embedded-rtos-sensor-hub](https://github.com/tahazarif10/embedded-rtos-sensor-hub) — bounded Zephyr RTOS sensor-pipeline work and deterministic fault injection.
- [Engineering Evidence](https://github.com/tahazarif10/tahazarif10/blob/main/EVIDENCE.md) — claim-level verification links across the portfolio.

## License

Apache-2.0.
