#include "amr_control_adapter/control_policy.hpp"

#include <algorithm>
#include <cmath>

namespace amr_control_adapter {
namespace {

bool finite_pose(const robotics_control::Pose2D& pose) noexcept
{
    return std::isfinite(pose.x) && std::isfinite(pose.y) && std::isfinite(pose.yaw);
}

bool finite_path(const std::vector<robotics_control::Vec2>& path) noexcept
{
    return std::all_of(
        path.cbegin(), path.cend(),
        [](const robotics_control::Vec2& point) {
            return std::isfinite(point.x) && std::isfinite(point.y);
        });
}

ControlDecision stop(StopReason reason) noexcept
{
    return {
        .twist = {},
        .stop_reason = reason,
        .motion_enabled = false,
    };
}

}  // namespace

bool valid_config(const ControlPolicyConfig& config) noexcept
{
    return std::isfinite(config.stale_timeout_s) && config.stale_timeout_s > 0.0 &&
           std::isfinite(config.lookahead_m) && config.lookahead_m > 0.0 &&
           std::isfinite(config.max_linear_mps) && config.max_linear_mps > 0.0 &&
           std::isfinite(config.max_angular_rps) && config.max_angular_rps > 0.0 &&
           std::isfinite(config.goal_tolerance_m) && config.goal_tolerance_m > 0.0 &&
           std::isfinite(config.curvature_slowdown) && config.curvature_slowdown >= 0.0;
}

ControlPolicy::ControlPolicy(ControlPolicyConfig config)
    : config_(config),
      controller_({
          .lookahead_m = config.lookahead_m,
          .max_linear_mps = config.max_linear_mps,
          .max_angular_rps = config.max_angular_rps,
          .goal_tolerance_m = config.goal_tolerance_m,
          .curvature_slowdown = config.curvature_slowdown,
      })
{
}

ControlDecision ControlPolicy::evaluate(
    const robotics_control::Pose2D& pose,
    const std::vector<robotics_control::Vec2>& path,
    double odometry_age_s,
    double path_age_s)
{
    if (path.empty()) {
        controller_.reset();
        return stop(StopReason::missing_path);
    }

    if (!finite_pose(pose) || !finite_path(path) ||
        !std::isfinite(odometry_age_s) || !std::isfinite(path_age_s) ||
        odometry_age_s < 0.0 || path_age_s < 0.0) {
        controller_.reset();
        return stop(StopReason::invalid_input);
    }

    if (odometry_age_s > config_.stale_timeout_s) {
        controller_.reset();
        return stop(StopReason::stale_odometry);
    }

    if (path_age_s > config_.stale_timeout_s) {
        controller_.reset();
        return stop(StopReason::stale_path);
    }

    const auto command = controller_.update(pose, path);

    if (command.goal_reached) {
        return stop(StopReason::goal_reached);
    }

    if (!std::isfinite(command.twist.linear_mps) ||
        !std::isfinite(command.twist.angular_rps)) {
        controller_.reset();
        return stop(StopReason::invalid_input);
    }

    return {
        .twist = {
            .linear_mps = std::clamp(
                command.twist.linear_mps,
                -config_.max_linear_mps,
                config_.max_linear_mps),
            .angular_rps = std::clamp(
                command.twist.angular_rps,
                -config_.max_angular_rps,
                config_.max_angular_rps),
        },
        .stop_reason = StopReason::none,
        .motion_enabled = true,
    };
}

void ControlPolicy::reset() noexcept
{
    controller_.reset();
}

std::string_view to_string(StopReason reason) noexcept
{
    switch (reason) {
    case StopReason::none:
        return "none";
    case StopReason::missing_path:
        return "missing_path";
    case StopReason::missing_odometry:
        return "missing_odometry";
    case StopReason::stale_path:
        return "stale_path";
    case StopReason::stale_odometry:
        return "stale_odometry";
    case StopReason::invalid_input:
        return "invalid_input";
    case StopReason::goal_reached:
        return "goal_reached";
    }

    return "unknown";
}

}  // namespace amr_control_adapter
