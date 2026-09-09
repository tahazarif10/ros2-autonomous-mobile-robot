# ROS 2 Autonomous Mobile Robot

[![CI](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/workflows/ci.yml/badge.svg)](https://github.com/tahazarif10/ros2-autonomous-mobile-robot/actions/workflows/ci.yml)
[![ROS 2](https://img.shields.io/badge/ROS%202-Jazzy-22314E)](https://docs.ros.org/en/jazzy/)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

An engineering-oriented autonomous mobile robot stack built around **ROS 2, C++, Nav2, deterministic testing, and reusable control software**.

The project is intentionally developed in measurable slices. Each milestone must leave behind buildable code, automated verification, and reproducible evidence rather than a demo-only repository.

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

The lifecycle-aware control adapter consumes the existing
[`robotics-control-core`](https://github.com/tahazarif10/robotics-control-core)
library at a pinned commit rather than duplicating planner/controller math inside ROS 2 nodes.

## Repository layout

```text
.
├── src/
│   ├── amr_description/      # Robot model and geometry
│   ├── amr_bringup/          # Launch and system composition
│   └── amr_control_adapter/  # Lifecycle ROS 2 boundary around control core
├── docs/
│   ├── ARCHITECTURE.md
│   ├── CONTROL_ADAPTER.md
│   ├── VERIFICATION.md
│   └── ROADMAP.md
└── .github/workflows/
    └── ci.yml
```

## Current status

**v0.2 control-core adapter — complete**

The repository now has a reproducible ROS 2 baseline plus a C++ lifecycle adapter that consumes the standalone `robotics-control-core` library at a pinned commit. The adapter converts Path/Odometry inputs, publishes bounded `cmd_vel`, and enforces safe-stop behavior for stale, missing, or non-finite data. Hosted verification is recorded in [`docs/VERIFICATION.md`](docs/VERIFICATION.md).

No hardware-performance claims are made by this repository. Simulation and regression results will be reported only for the exact checked-in fixtures and configurations used to produce them.

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

See [Architecture](docs/ARCHITECTURE.md), [Control adapter contract](docs/CONTROL_ADAPTER.md), [Verification](docs/VERIFICATION.md), and [Roadmap](docs/ROADMAP.md).

## License

Apache-2.0.
