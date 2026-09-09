#pragma once

#include "amr_control_adapter/control_policy.hpp"

#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp_lifecycle/lifecycle_publisher.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace amr_control_adapter {

class ControlAdapterNode final : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit ControlAdapterNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

    using CallbackReturn =
        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

    CallbackReturn on_configure(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_activate(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_cleanup(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_shutdown(const rclcpp_lifecycle::State& state) override;

private:
    using SteadyClock = std::chrono::steady_clock;

    void on_path(const nav_msgs::msg::Path::SharedPtr message);
    void on_odometry(const nav_msgs::msg::Odometry::SharedPtr message);
    void on_control_tick();

    void publish_stop();
    void publish_diagnostics(
        StopReason reason,
        bool motion_enabled,
        std::optional<double> odometry_age_s,
        std::optional<double> path_age_s);
    void clear_runtime_state();

    static robotics_control::Pose2D pose_from_odometry(
        const nav_msgs::msg::Odometry& message);

    static std::vector<robotics_control::Vec2> path_from_message(
        const nav_msgs::msg::Path& message);

    ControlPolicyConfig load_config() const;

    mutable std::mutex mutex_;
    std::optional<robotics_control::Pose2D> pose_;
    std::vector<robotics_control::Vec2> path_;
    std::optional<SteadyClock::time_point> odometry_received_at_;
    std::optional<SteadyClock::time_point> path_received_at_;

    std::unique_ptr<ControlPolicy> policy_;

    rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr path_subscription_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odometry_subscription_;
    rclcpp_lifecycle::LifecyclePublisher<geometry_msgs::msg::Twist>::SharedPtr
        command_publisher_;
    rclcpp_lifecycle::LifecyclePublisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr
        diagnostics_publisher_;
    rclcpp::TimerBase::SharedPtr control_timer_;

    double control_rate_hz_{20.0};
    std::uint64_t control_sequence_{0};
};

}  // namespace amr_control_adapter
