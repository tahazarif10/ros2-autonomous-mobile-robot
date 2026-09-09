#include "amr_control_adapter/control_policy.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <vector>

namespace {

using amr_control_adapter::ControlPolicy;
using amr_control_adapter::ControlPolicyConfig;
using amr_control_adapter::StopReason;
using robotics_control::Pose2D;
using robotics_control::Vec2;

const std::vector<Vec2> kStraightPath{
    {0.0, 0.0},
    {1.0, 0.0},
    {2.0, 0.0},
};

TEST(ControlPolicy, RejectsInvalidConfiguration)
{
    auto config = ControlPolicyConfig{};
    config.max_linear_mps = 0.0;
    EXPECT_FALSE(amr_control_adapter::valid_config(config));
}

TEST(ControlPolicy, StopReasonStringsAreStable)
{
    EXPECT_EQ(
        amr_control_adapter::to_string(StopReason::missing_odometry),
        "missing_odometry");
    EXPECT_EQ(
        amr_control_adapter::to_string(StopReason::missing_path),
        "missing_path");
}

TEST(ControlPolicy, StopsWithoutPath)
{
    ControlPolicy policy(ControlPolicyConfig{});

    const auto decision = policy.evaluate(
        Pose2D{0.0, 0.0, 0.0},
        {},
        0.0,
        0.0);

    EXPECT_FALSE(decision.motion_enabled);
    EXPECT_EQ(decision.stop_reason, StopReason::missing_path);
    EXPECT_DOUBLE_EQ(decision.twist.linear_mps, 0.0);
    EXPECT_DOUBLE_EQ(decision.twist.angular_rps, 0.0);
}

TEST(ControlPolicy, StopsOnStaleOdometry)
{
    ControlPolicy policy(ControlPolicyConfig{.stale_timeout_s = 0.5});

    const auto decision = policy.evaluate(
        Pose2D{0.0, 0.0, 0.0},
        kStraightPath,
        0.51,
        0.1);

    EXPECT_FALSE(decision.motion_enabled);
    EXPECT_EQ(decision.stop_reason, StopReason::stale_odometry);
}

TEST(ControlPolicy, StopsOnStalePath)
{
    ControlPolicy policy(ControlPolicyConfig{.stale_timeout_s = 0.5});

    const auto decision = policy.evaluate(
        Pose2D{0.0, 0.0, 0.0},
        kStraightPath,
        0.1,
        0.51);

    EXPECT_FALSE(decision.motion_enabled);
    EXPECT_EQ(decision.stop_reason, StopReason::stale_path);
}

TEST(ControlPolicy, StopsOnInvalidPose)
{
    ControlPolicy policy(ControlPolicyConfig{});
    auto pose = Pose2D{0.0, 0.0, 0.0};
    pose.x = std::numeric_limits<double>::quiet_NaN();

    const auto decision = policy.evaluate(pose, kStraightPath, 0.1, 0.1);

    EXPECT_FALSE(decision.motion_enabled);
    EXPECT_EQ(decision.stop_reason, StopReason::invalid_input);
}

TEST(ControlPolicy, ProducesBoundedMotionForFreshInputs)
{
    ControlPolicyConfig config;
    config.max_linear_mps = 0.40;
    config.max_angular_rps = 0.60;

    ControlPolicy policy(config);

    const auto decision = policy.evaluate(
        Pose2D{0.0, 0.0, 0.0},
        kStraightPath,
        0.1,
        0.1);

    EXPECT_TRUE(decision.motion_enabled);
    EXPECT_EQ(decision.stop_reason, StopReason::none);
    EXPECT_GT(decision.twist.linear_mps, 0.0);
    EXPECT_LE(std::abs(decision.twist.linear_mps), config.max_linear_mps);
    EXPECT_LE(std::abs(decision.twist.angular_rps), config.max_angular_rps);
}

TEST(ControlPolicy, StopsAtGoal)
{
    ControlPolicy policy(ControlPolicyConfig{.goal_tolerance_m = 0.10});

    const auto decision = policy.evaluate(
        Pose2D{2.0, 0.0, 0.0},
        kStraightPath,
        0.1,
        0.1);

    EXPECT_FALSE(decision.motion_enabled);
    EXPECT_EQ(decision.stop_reason, StopReason::goal_reached);
    EXPECT_DOUBLE_EQ(decision.twist.linear_mps, 0.0);
    EXPECT_DOUBLE_EQ(decision.twist.angular_rps, 0.0);
}

}  // namespace
