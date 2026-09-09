#include "amr_control_adapter/control_adapter_node.hpp"

#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <gtest/gtest.h>
#include <lifecycle_msgs/msg/state.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <rclcpp/rclcpp.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;

class ControlAdapterFaultFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (!rclcpp::ok()) {
            int argc = 0;
            rclcpp::init(argc, nullptr);
        }
    }

    static void TearDownTestSuite()
    {
        if (rclcpp::ok()) {
            rclcpp::shutdown();
        }
    }

    void SetUp() override
    {
        adapter_ = std::make_shared<amr_control_adapter::ControlAdapterNode>();
        helper_ = std::make_shared<rclcpp::Node>("control_adapter_fault_test");

        ASSERT_TRUE(
            adapter_->set_parameter(
                rclcpp::Parameter("control_rate_hz", 50.0))
                .successful);
        ASSERT_TRUE(
            adapter_->set_parameter(
                rclcpp::Parameter("stale_timeout_s", 0.15))
                .successful);

        path_pub_ = helper_->create_publisher<nav_msgs::msg::Path>(
            "plan",
            rclcpp::QoS(1).reliable().transient_local());
        odom_pub_ = helper_->create_publisher<nav_msgs::msg::Odometry>(
            "odom",
            rclcpp::SensorDataQoS());

        cmd_sub_ = helper_->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel",
            rclcpp::QoS(10).reliable(),
            [this](geometry_msgs::msg::Twist::SharedPtr message) {
                std::scoped_lock lock(observation_mutex_);
                latest_command_ = *message;
                command_seen_ = true;
            });

        diagnostics_sub_ =
            helper_->create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
                "diagnostics",
                rclcpp::QoS(10).reliable(),
                [this](diagnostic_msgs::msg::DiagnosticArray::SharedPtr message) {
                    if (message->status.empty()) {
                        return;
                    }

                    std::scoped_lock lock(observation_mutex_);
                    latest_reason_ = message->status.front().message;
                    diagnostics_seen_ = true;
                });

        executor_.add_node(adapter_->get_node_base_interface());
        executor_.add_node(helper_);

        ASSERT_EQ(
            adapter_->configure().id(),
            lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE);
        ASSERT_EQ(
            adapter_->activate().id(),
            lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE);

        spin_thread_ = std::thread([this]() { executor_.spin(); });

        ASSERT_TRUE(wait_until(
            [this]() {
                return path_pub_->get_subscription_count() > 0 &&
                       odom_pub_->get_subscription_count() > 0;
            },
            2s));
    }

    void TearDown() override
    {
        if (adapter_->get_current_state().id() ==
            lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            adapter_->deactivate();
        }
        if (adapter_->get_current_state().id() ==
            lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE) {
            adapter_->cleanup();
        }

        executor_.cancel();
        if (spin_thread_.joinable()) {
            spin_thread_.join();
        }

        executor_.remove_node(helper_);
        executor_.remove_node(adapter_->get_node_base_interface());

        diagnostics_sub_.reset();
        cmd_sub_.reset();
        path_pub_.reset();
        odom_pub_.reset();
        helper_.reset();
        adapter_.reset();
    }

    template <typename Predicate>
    bool wait_until(Predicate&& predicate, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate()) {
                return true;
            }
            std::this_thread::sleep_for(10ms);
        }
        return predicate();
    }

    nav_msgs::msg::Path make_path() const
    {
        nav_msgs::msg::Path message;
        message.header.frame_id = "map";

        for (double x : {0.0, 1.0, 2.0}) {
            geometry_msgs::msg::PoseStamped pose;
            pose.header.frame_id = "map";
            pose.pose.position.x = x;
            pose.pose.orientation.w = 1.0;
            message.poses.push_back(pose);
        }

        return message;
    }

    nav_msgs::msg::Odometry make_odometry(double x = 0.0) const
    {
        nav_msgs::msg::Odometry message;
        message.header.frame_id = "odom";
        message.child_frame_id = "base_link";
        message.pose.pose.position.x = x;
        message.pose.pose.orientation.w = 1.0;
        return message;
    }

    void publish_fresh_inputs()
    {
        path_pub_->publish(make_path());
        odom_pub_->publish(make_odometry());
    }

    bool has_reason(const std::string& reason)
    {
        std::scoped_lock lock(observation_mutex_);
        return diagnostics_seen_ && latest_reason_ == reason;
    }

    bool has_motion()
    {
        std::scoped_lock lock(observation_mutex_);
        return command_seen_ &&
               latest_command_.linear.x > 0.01 &&
               std::abs(latest_command_.angular.z) < 2.0;
    }

    bool has_zero_command()
    {
        std::scoped_lock lock(observation_mutex_);
        return command_seen_ &&
               std::abs(latest_command_.linear.x) < 1.0e-9 &&
               std::abs(latest_command_.angular.z) < 1.0e-9;
    }

    std::shared_ptr<amr_control_adapter::ControlAdapterNode> adapter_;
    rclcpp::Node::SharedPtr helper_;
    rclcpp::executors::MultiThreadedExecutor executor_;
    std::thread spin_thread_;

    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
    rclcpp::Subscription<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr
        diagnostics_sub_;

    std::mutex observation_mutex_;
    geometry_msgs::msg::Twist latest_command_;
    std::string latest_reason_;
    bool command_seen_{false};
    bool diagnostics_seen_{false};
};

TEST_F(ControlAdapterFaultFixture, ReportsFaultReasonsAndPublishesSafeStop)
{
    publish_fresh_inputs();

    ASSERT_TRUE(wait_until(
        [this]() { return has_motion() && has_reason("none"); },
        2s));

    auto invalid_odometry = make_odometry();
    invalid_odometry.pose.pose.position.x =
        std::numeric_limits<double>::quiet_NaN();
    odom_pub_->publish(invalid_odometry);

    ASSERT_TRUE(wait_until(
        [this]() {
            return has_zero_command() && has_reason("invalid_input");
        },
        2s));

    publish_fresh_inputs();
    ASSERT_TRUE(wait_until(
        [this]() { return has_motion() && has_reason("none"); },
        2s));

    ASSERT_TRUE(wait_until(
        [this]() {
            return has_zero_command() && has_reason("stale_odometry");
        },
        2s));

    publish_fresh_inputs();
    ASSERT_TRUE(wait_until(
        [this]() { return has_motion() && has_reason("none"); },
        2s));

    for (int i = 0; i < 7; ++i) {
        odom_pub_->publish(make_odometry());
        std::this_thread::sleep_for(50ms);
    }

    ASSERT_TRUE(wait_until(
        [this]() {
            return has_zero_command() && has_reason("stale_path");
        },
        2s));
}

}  // namespace
