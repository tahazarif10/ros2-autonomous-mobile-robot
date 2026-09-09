# Roadmap

The roadmap is evidence-driven. A milestone is complete only when its implementation, tests, and reproducible verification are on `main`.

## v0.1 — Reproducible ROS 2 baseline

- [x] public Apache-2.0 repository
- [x] ROS 2 Jazzy / Ubuntu 24.04 baseline
- [x] workspace structure
- [x] differential-drive Xacro description
- [x] display launch
- [x] GitHub Actions build/test workflow
- [x] description validation test
- [x] launch smoke test
- [x] CI evidence recorded in README/docs

## v0.2 — Control-core adapter

- [x] define adapter input/output contract
- [x] consume `robotics-control-core` without copying algorithm code
- [x] path + odometry conversion
- [x] bounded `cmd_vel` output
- [x] stale input and invalid-data handling
- [x] lifecycle support
- [x] unit and lifecycle integration tests

## v0.3 — Navigation integration

- [x] Nav2 bringup
- [x] explicit QoS contracts
- [x] TF ownership validation
- [x] map and localization configuration
- [x] deterministic navigation scenario
- [x] collision/goal assertions
- [x] merged-main CI evidence recorded

## v0.4 — Replay and observability

- [x] rosbag2 sqlite3 replay fixture generated from checked-in source data
- [x] deterministic two-pass replay regression with equal canonical outcomes
- [x] lifecycle control diagnostics with stable stop reasons
- [x] replay timing and controller metrics
- [x] stale odometry/path and non-finite input fault injection
- [x] missing-TF navigation fail-closed regression
- [x] concurrent ROS test-graph isolation
- [x] merged-main CI evidence recorded

## v0.5 — Simulation qualification

- [ ] reproducible simulator world
- [ ] repeatable start/goal suite
- [ ] baseline metrics stored with commit metadata
- [ ] CI or scheduled simulation regression where practical

## v1.0 — Portfolio-grade autonomous stack

- [ ] documented architecture and interfaces
- [ ] all required CI gates green
- [ ] reproducible demo
- [ ] benchmark/verification report
- [ ] clear limitations and no unsupported hardware claims
