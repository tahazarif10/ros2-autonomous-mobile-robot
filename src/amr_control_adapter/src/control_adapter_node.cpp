#include "amr_control_adapter/control_adapter_node.hpp"

#include <lifecycle_msgs/msg/state.hpp>

#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace amr_control_adapter {

ControlAdapterNode::ControlAdapterNode(const rclcpp::NodeOptions& options)
    : rclcpp_lifecycle::LifecycleNode("amr_control_adapter", options)
{
    declare_parameter("control_rate_hz", 20.0);
    declare_parameter("stale_timeout_s", 0.5);
    declare_parameter("lookahead_m", 0.55);
    declare_parameter("max_linear_mps", 0.85);
    declare_parameter("max_angular_rps", 1.8);
    declare_parameter("goal_tolerance_m", 0.10);
    declare_parameter("curvature_slowdown", 0.70);
}

ControlPolicyConfig ControlAdapterNode::load_config() const
{
    return {
        .stale_timeout_s = get_parameter("stale_timeout_s").as_double(),
        .lookahead_m = get_parameter("lookahead_m").as_double(),
        .max_linear_mps = get_parameter("max_linear_mps").as_double(),
        .max_angular_rps = get_parameter("max_angular_rps").as_double(),
        .goal_tolerance_m = get_parameter("goal_tolerance_m").as_double(),
        .curvature_slowdown = get_parameter("curvature_slowdown").as_double(),
    };
}

ControlAdapterNode::CallbackReturn ControlAdapterNode::on_configure(
    const rclcpp_lifecycle::State&)
{
    control_rate_hz_ = get_parameter("control_rate_hz").as_double();
    const auto config = load_config();

    if (!std::isfinite(control_rate_hz_) || control_rate_hz_ <= 0.0 ||
        !valid_config(config)) {
        RCLCPP_ERROR(get_logger(), "Invalid control adapter configuration");
        return CallbackReturn::FAILURE;
    }

    policy_ = std::make_unique<ControlPolicy>(config);

    command_publisher_ = create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    diagnostics_publisher_ =
        create_publisher<diagnostic_msgs::msg::DiagnosticArray>("diagnostics", 10);

    path_subscription_ = create_subscription<nav_msgs::msg::Path>(
        "plan",
        rclcpp::QoS(1).reliable().transient_local(),
        std::bind(&ControlAdapterNode::on_path, this, std::placeholders::_1));

    odometry_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
        "odom",
        rclcpp::SensorDataQoS(),
        std::bind(&ControlAdapterNode::on_odometry, this, std::placeholders::_1));

    clear_runtime_state();

    RCLCPP_INFO(get_logger(), "Configured control adapter");
    return CallbackReturn::SUCCESS;
}

ControlAdapterNode::CallbackReturn ControlAdapterNode::on_activate(
    const rclcpp_lifecycle::State&)
{
    if (!command_publisher_ || !diagnostics_publisher_ || !policy_) {
        RCLCPP_ERROR(get_logger(), "Cannot activate before configuration");
        return CallbackReturn::FAILURE;
    }

    command_publisher_->on_activate();
    diagnostics_publisher_->on_activate();

    const auto period = std::chrono::duration<double>(1.0 / control_rate_hz_);
    control_timer_ = create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(period),
        std::bind(&ControlAdapterNode::on_control_tick, this));

    RCLCPP_INFO(get_logger(), "Activated control adapter");
    return CallbackReturn::SUCCESS;
}

ControlAdapterNode::CallbackReturn ControlAdapterNode::on_deactivate(
    const rclcpp_lifecycle::State&)
{
    if (control_timer_) {
        control_timer_->cancel();
        control_timer_.reset();
    }

    publish_stop();

    if (diagnostics_publisher_) {
        diagnostics_publisher_->on_deactivate();
    }

    if (command_publisher_) {
        command_publisher_->on_deactivate();
    }

    if (policy_) {
        policy_->reset();
    }

    RCLCPP_INFO(get_logger(), "Deactivated control adapter");
    return CallbackReturn::SUCCESS;
}

ControlAdapterNode::CallbackReturn ControlAdapterNode::on_cleanup(
    const rclcpp_lifecycle::State&)
{
    if (control_timer_) {
        control_timer_->cancel();
        control_timer_.reset();
    }

    clear_runtime_state();
    policy_.reset();
    path_subscription_.reset();
    odometry_subscription_.reset();
    command_publisher_.reset();
    diagnostics_publisher_.reset();

    RCLCPP_INFO(get_logger(), "Cleaned up control adapter");
    return CallbackReturn::SUCCESS;
}

ControlAdapterNode::CallbackReturn ControlAdapterNode::on_shutdown(
    const rclcpp_lifecycle::State&)
{
    if (control_timer_) {
        control_timer_->cancel();
        control_timer_.reset();
    }

    clear_runtime_state();
    policy_.reset();

    RCLCPP_INFO(get_logger(), "Shutdown control adapter");
    return CallbackReturn::SUCCESS;
}

void ControlAdapterNode::on_path(const nav_msgs::msg::Path::SharedPtr message)
{
    auto converted = path_from_message(*message);

    std::scoped_lock lock(mutex_);
    path_ = std::move(converted);
    path_received_at_ = SteadyClock::now();

    if (policy_) {
        policy_->reset();
    }
}

void ControlAdapterNode::on_odometry(const nav_msgs::msg::Odometry::SharedPtr message)
{
    const auto converted = pose_from_odometry(*message);

    std::scoped_lock lock(mutex_);
    pose_ = converted;
    odometry_received_at_ = SteadyClock::now();
}

void ControlAdapterNode::on_control_tick()
{
    std::optional<robotics_control::Pose2D> pose;
    std::vector<robotics_control::Vec2> path;
    std::optional<SteadyClock::time_point> odometry_received_at;
    std::optional<SteadyClock::time_point> path_received_at;

    {
        std::scoped_lock lock(mutex_);
        pose = pose_;
        path = path_;
        odometry_received_at = odometry_received_at_;
        path_received_at = path_received_at_;
    }

    if (!policy_) {
        publish_stop();
        publish_diagnostics(StopReason::invalid_input, false);
        return;
    }

    if (!pose || !odometry_received_at) {
        publish_stop();
        publish_diagnostics(StopReason::missing_odometry, false);
        return;
    }

    if (path.empty() || !path_received_at) {
        publish_stop();
        publish_diagnostics(StopReason::missing_path, false);
        return;
    }

    const auto now = SteadyClock::now();
    const auto odometry_age =
        std::chrono::duration<double>(now - *odometry_received_at).count();
    const auto path_age =
        std::chrono::duration<double>(now - *path_received_at).count();

    const auto decision = policy_->evaluate(*pose, path, odometry_age, path_age);

    geometry_msgs::msg::Twist command;
    command.linear.x = decision.twist.linear_mps;
    command.angular.z = decision.twist.angular_rps;

    if (command_publisher_ && command_publisher_->is_activated()) {
        command_publisher_->publish(command);
    }

    publish_diagnostics(
        decision.stop_reason,
        decision.motion_enabled,
        odometry_age,
        path_age);
}

void ControlAdapterNode::publish_stop()
{
    if (!command_publisher_ || !command_publisher_->is_activated()) {
        return;
    }

    command_publisher_->publish(geometry_msgs::msg::Twist{});
}

void ControlAdapterNode::publish_diagnostics(
    StopReason reason,
    bool motion_enabled,
    std::optional<double> odometry_age_s,
    std::optional<double> path_age_s)
{
    if (!diagnostics_publisher_ || !diagnostics_publisher_->is_activated()) {
        return;
    }

    diagnostic_msgs::msg::DiagnosticArray array;
    array.header.stamp = now();

    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "amr_control_adapter/control_policy";
    status.hardware_id = "none";
    status.level =
        (reason == StopReason::none || reason == StopReason::goal_reached)
            ? diagnostic_msgs::msg::DiagnosticStatus::OK
            : diagnostic_msgs::msg::DiagnosticStatus::WARN;
    status.message = std::string(to_string(reason));

    diagnostic_msgs::msg::KeyValue motion_value;
    motion_value.key = "motion_enabled";
    motion_value.value = motion_enabled ? "true" : "false";
    status.values.push_back(std::move(motion_value));

    if (odometry_age_s) {
        diagnostic_msgs::msg::KeyValue age_value;
        age_value.key = "odometry_age_s";
        age_value.value = std::to_string(*odometry_age_s);
        status.values.push_back(std::move(age_value));
    }

    if (path_age_s) {
        diagnostic_msgs::msg::KeyValue age_value;
        age_value.key = "path_age_s";
        age_value.value = std::to_string(*path_age_s);
        status.values.push_back(std::move(age_value));
    }

    array.status.push_back(std::move(status));
    diagnostics_publisher_->publish(array);
}

void ControlAdapterNode::clear_runtime_state()
{
    std::scoped_lock lock(mutex_);
    pose_.reset();
    path_.clear();
    odometry_received_at_.reset();
    path_received_at_.reset();
}

robotics_control::Pose2D ControlAdapterNode::pose_from_odometry(
    const nav_msgs::msg::Odometry& message)
{
    const auto& position = message.pose.pose.position;
    const auto& orientation = message.pose.pose.orientation;

    const double sin_yaw =
        2.0 * (orientation.w * orientation.z + orientation.x * orientation.y);
    const double cos_yaw =
        1.0 - 2.0 * (orientation.y * orientation.y + orientation.z * orientation.z);

    return {
        .x = position.x,
        .y = position.y,
        .yaw = std::atan2(sin_yaw, cos_yaw),
    };
}

std::vector<robotics_control::Vec2> ControlAdapterNode::path_from_message(
    const nav_msgs::msg::Path& message)
{
    std::vector<robotics_control::Vec2> path;
    path.reserve(message.poses.size());

    for (const auto& pose_stamped : message.poses) {
        path.push_back({
            .x = pose_stamped.pose.position.x,
            .y = pose_stamped.pose.position.y,
        });
    }

    return path;
}

}  // namespace amr_control_adapter
