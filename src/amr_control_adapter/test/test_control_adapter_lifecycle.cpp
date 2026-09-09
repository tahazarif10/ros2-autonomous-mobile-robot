#include "amr_control_adapter/control_adapter_node.hpp"

#include <gtest/gtest.h>
#include <lifecycle_msgs/msg/state.hpp>
#include <rclcpp/rclcpp.hpp>

#include <memory>

namespace {

class RclcppFixture : public ::testing::Test {
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
};

TEST_F(RclcppFixture, CompletesNominalLifecycleTransitions)
{
    using lifecycle_msgs::msg::State;

    auto node = std::make_shared<amr_control_adapter::ControlAdapterNode>();

    EXPECT_EQ(
        node->get_current_state().id(),
        State::PRIMARY_STATE_UNCONFIGURED);

    EXPECT_EQ(
        node->configure().id(),
        State::PRIMARY_STATE_INACTIVE);

    EXPECT_EQ(
        node->activate().id(),
        State::PRIMARY_STATE_ACTIVE);

    EXPECT_EQ(
        node->deactivate().id(),
        State::PRIMARY_STATE_INACTIVE);

    EXPECT_EQ(
        node->cleanup().id(),
        State::PRIMARY_STATE_UNCONFIGURED);
}

TEST_F(RclcppFixture, RejectsInvalidRuntimeConfiguration)
{
    using lifecycle_msgs::msg::State;

    auto node = std::make_shared<amr_control_adapter::ControlAdapterNode>();
    const auto result = node->set_parameter(
        rclcpp::Parameter("max_linear_mps", 0.0));

    ASSERT_TRUE(result.successful);

    const auto state = node->configure();
    EXPECT_EQ(state.id(), State::PRIMARY_STATE_UNCONFIGURED);
}

}  // namespace
