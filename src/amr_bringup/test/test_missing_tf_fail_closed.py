import time
import unittest

from geometry_msgs.msg import Twist
import launch
import launch_testing
from launch.actions import ExecuteProcess
from lifecycle_msgs.msg import State
from lifecycle_msgs.srv import GetState
import pytest
import rclpy
from tf2_ros import Buffer, TransformListener


@pytest.mark.launch_test
def generate_test_description():
    navigation = ExecuteProcess(
        cmd=[
            "ros2",
            "launch",
            "amr_bringup",
            "navigation_loopback.launch.py",
            "start_loopback:=false",
        ],
        output="screen",
    )

    return (
        launch.LaunchDescription(
            [
                navigation,
                launch_testing.actions.ReadyToTest(),
            ]
        ),
        {"navigation": navigation},
    )


class TestMissingTfFailsClosed(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def setUp(self):
        self.node = rclpy.create_node("missing_tf_fail_closed_test")
        self.nonzero_command_seen = False

        self.cmd_sub = self.node.create_subscription(
            Twist,
            "/cmd_vel",
            self._on_cmd,
            10,
        )
        self.state_client = self.node.create_client(
            GetState,
            "/bt_navigator/get_state",
        )
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(
            self.tf_buffer,
            self.node,
            spin_thread=False,
        )

    def tearDown(self):
        self.node.destroy_subscription(self.cmd_sub)
        self.node.destroy_client(self.state_client)
        self.node.destroy_node()

    def _on_cmd(self, message):
        if abs(message.linear.x) > 1.0e-9 or abs(message.angular.z) > 1.0e-9:
            self.nonzero_command_seen = True

    def _spin_until(self, predicate, timeout_s):
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            rclpy.spin_once(self.node, timeout_sec=0.1)
            if predicate():
                return True
        return predicate()

    def _current_bt_state(self):
        request = GetState.Request()
        future = self.state_client.call_async(request)
        if not self._spin_until(lambda: future.done(), 5.0):
            self.fail("Timed out reading bt_navigator lifecycle state")

        response = future.result()
        self.assertIsNotNone(response)
        return response.current_state

    def test_missing_global_tf_prevents_active_navigation_and_motion(self, navigation):
        self.assertTrue(
            self.state_client.wait_for_service(timeout_sec=20.0),
            "bt_navigator lifecycle service did not become available",
        )

        # Without the loopback simulator there is intentionally no map->odom
        # or odom->base_link transform. The navigation lifecycle may configure,
        # but it must not become operational and command motion.
        deadline = time.monotonic() + 8.0
        observed_states = set()

        while time.monotonic() < deadline:
            state = self._current_bt_state()
            observed_states.add((state.id, state.label))
            self.assertNotEqual(
                state.id,
                State.PRIMARY_STATE_ACTIVE,
                f"bt_navigator unexpectedly became ACTIVE: {observed_states}",
            )
            rclpy.spin_once(self.node, timeout_sec=0.25)

        self.assertFalse(
            self.tf_buffer.can_transform(
                "map",
                "base_link",
                rclpy.time.Time(),
            ),
            "map -> base_link unexpectedly existed with loopback disabled",
        )
        self.assertFalse(
            self.nonzero_command_seen,
            "Navigation published non-zero cmd_vel while required TF was missing",
        )


@launch_testing.post_shutdown_test()
class TestProcessExit(unittest.TestCase):
    def test_navigation_process_exits(self, proc_info, navigation):
        launch_testing.asserts.assertExitCodes(proc_info, process=navigation)
