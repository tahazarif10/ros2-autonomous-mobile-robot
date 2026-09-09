#pragma once

#include "robotics_control/pure_pursuit.hpp"
#include "robotics_control/types.hpp"

#include <string_view>
#include <vector>

namespace amr_control_adapter {

enum class StopReason {
    none,
    missing_path,
    missing_odometry,
    stale_path,
    stale_odometry,
    invalid_input,
    goal_reached,
};

struct ControlPolicyConfig {
    double stale_timeout_s{0.5};
    double lookahead_m{0.55};
    double max_linear_mps{0.85};
    double max_angular_rps{1.8};
    double goal_tolerance_m{0.10};
    double curvature_slowdown{0.70};
};

struct ControlDecision {
    robotics_control::BodyTwist twist{};
    StopReason stop_reason{StopReason::none};
    bool motion_enabled{false};
};

class ControlPolicy {
public:
    explicit ControlPolicy(ControlPolicyConfig config);

    [[nodiscard]] ControlDecision evaluate(
        const robotics_control::Pose2D& pose,
        const std::vector<robotics_control::Vec2>& path,
        double odometry_age_s,
        double path_age_s);

    void reset() noexcept;

private:
    ControlPolicyConfig config_;
    robotics_control::PurePursuitController controller_;
};

[[nodiscard]] bool valid_config(const ControlPolicyConfig& config) noexcept;
[[nodiscard]] std::string_view to_string(StopReason reason) noexcept;

}  // namespace amr_control_adapter
