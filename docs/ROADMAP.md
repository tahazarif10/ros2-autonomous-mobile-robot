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

- [ ] define adapter input/output contract
- [ ] consume `robotics-control-core` without copying algorithm code
- [ ] path + odometry conversion
- [ ] bounded `cmd_vel` output
- [ ] stale input and invalid-data handling
- [ ] lifecycle support
- [ ] unit and integration tests

## v0.3 — Navigation integration

- [ ] Nav2 bringup
- [ ] explicit QoS contracts
- [ ] TF ownership validation
- [ ] map and localization configuration
- [ ] deterministic navigation scenario
- [ ] collision/goal assertions

## v0.4 — Replay and observability

- [ ] rosbag replay fixture
- [ ] deterministic regression runner
- [ ] diagnostics
- [ ] timing and controller metrics
- [ ] failure injection for stale/missing inputs

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
