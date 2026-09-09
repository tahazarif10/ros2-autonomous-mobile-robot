import math
import subprocess
import time
import unittest

from action_msgs.msg import GoalStatus
from geometry_msgs.msg import PoseWithCovarianceStamped
from lifecycle_msgs.msg import State
from lifecycle_msgs.srv import GetState
from nav2_msgs.action import NavigateToPose
from nav_msgs.msg import Odometry
import launch
import launch_testing
from launch.actions import ExecuteProcess
import pytest
import rclpy
from rclpy.action import ActionClient
from tf2_ros import Buffer, TransformListener


START_X = -2.0
START_Y = 0.0
GOAL_X = 2.0
GOAL_Y = 0.0

# Central occupied rectangle is approximately x=[-0.40, 0.40],
# y=[-1.00, 1.00]. Expand it by the 0.20 m robot radius.
FORBIDDEN_X = (-0.60, 0.60)
FORBIDDEN_Y = (-1.20, 1.20)


@pytest.mark.launch_test
def generate_test_description():
    navigation = ExecuteProcess(
        cmd=[
            "ros2",
            "launch",
            "amr_bringup",
            "navigation_loopback.launch.py",
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


class TestDeterministicNavigationFixture(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def setUp(self):
        self.node = rclpy.create_node("navigation_fixture_test")
        self.trajectory = []

        self.initial_pose_pub = self.node.create_publisher(
            PoseWithCovarianceStamped,
            "/initialpose",
            10,
        )
        self.odom_sub = self.node.create_subscription(
            Odometry,
            "/odom",
            self._odom_callback,
            20,
        )
        self.client = ActionClient(
            self.node,
            NavigateToPose,
            "/navigate_to_pose",
        )
        self.lifecycle_client = self.node.create_client(
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
        self.node.destroy_subscription(self.odom_sub)
        self.node.destroy_publisher(self.initial_pose_pub)
        self.client.destroy()
        self.node.destroy_client(self.lifecycle_client)
        self.node.destroy_node()

    def _odom_callback(self, msg):
        # Loopback initial pose is represented by map->odom. With the
        # zero-yaw fixture, map coordinates are initial translation + odom.
        self.trajectory.append(
            (
                START_X + msg.pose.pose.position.x,
                START_Y + msg.pose.pose.position.y,
            )
        )

    def _spin_until(self, predicate, timeout):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            rclpy.spin_once(self.node, timeout_sec=0.1)
            if predicate():
                return True
        return False

    def _wait_for_bt_navigator_active(self):
        self.assertTrue(
            self.lifecycle_client.wait_for_service(timeout_sec=20.0),
            "bt_navigator lifecycle service did not become available",
        )

        deadline = time.monotonic() + 30.0
        last_state = "unknown"

        while time.monotonic() < deadline:
            future = self.lifecycle_client.call_async(GetState.Request())
            if self._spin_until(lambda: future.done(), 5.0):
                response = future.result()
                if response is not None:
                    last_state = response.current_state.label
                    if response.current_state.id == State.PRIMARY_STATE_ACTIVE:
                        return
            time.sleep(0.1)

        self.fail(
            "bt_navigator did not reach ACTIVE state; "
            f"last observed state: {last_state}"
        )

    def _assert_tf_chain(self):
        expected_edges = [
            ("map", "odom"),
            ("odom", "base_link"),
            ("base_link", "base_scan"),
        ]
        deadline = time.monotonic() + 20.0

        while time.monotonic() < deadline:
            rclpy.spin_once(self.node, timeout_sec=0.1)
            if all(
                self.tf_buffer.can_transform(
                    target,
                    source,
                    rclpy.time.Time(),
                )
                for target, source in expected_edges
            ):
                return

        missing = [
            f"{target} <- {source}"
            for target, source in expected_edges
            if not self.tf_buffer.can_transform(
                target,
                source,
                rclpy.time.Time(),
            )
        ]
        self.fail(f"Expected TF chain was not available: {missing}")

    def _publish_initial_pose(self):
        discovery_deadline = time.monotonic() + 20.0
        while (
            self.initial_pose_pub.get_subscription_count() == 0
            and time.monotonic() < discovery_deadline
        ):
            rclpy.spin_once(self.node, timeout_sec=0.1)

        self.assertGreater(
            self.initial_pose_pub.get_subscription_count(),
            0,
            "Loopback simulator did not subscribe to /initialpose",
        )

        msg = PoseWithCovarianceStamped()
        msg.header.frame_id = "map"
        msg.header.stamp = self.node.get_clock().now().to_msg()
        msg.pose.pose.position.x = START_X
        msg.pose.pose.position.y = START_Y
        msg.pose.pose.orientation.w = 1.0

        # Publish more than once after discovery so TF is established before
        # Nav2 lifecycle activation requires map -> base_link.
        for _ in range(5):
            msg.header.stamp = self.node.get_clock().now().to_msg()
            self.initial_pose_pub.publish(msg)
            rclpy.spin_once(self.node, timeout_sec=0.20)

    def test_navigation_reaches_goal_without_entering_obstacle(self, navigation):
        # Confirm the launched process did not fail during startup.
        time.sleep(1.0)
        result = subprocess.run(
            ["ros2", "node", "list"],
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

        # Establish map -> odom before waiting for the navigation stack to
        # become active. Planner/controller costmaps need that TF during
        # lifecycle activation.
        self._publish_initial_pose()
        self._assert_tf_chain()

        self._wait_for_bt_navigator_active()

        self.assertTrue(
            self.client.wait_for_server(timeout_sec=20.0),
            "NavigateToPose action server did not become available",
        )

        goal = NavigateToPose.Goal()
        goal.pose.header.frame_id = "map"
        goal.pose.header.stamp = self.node.get_clock().now().to_msg()
        goal.pose.pose.position.x = GOAL_X
        goal.pose.pose.position.y = GOAL_Y
        goal.pose.pose.orientation.w = 1.0

        send_future = self.client.send_goal_async(goal)
        self.assertTrue(
            self._spin_until(lambda: send_future.done(), 10.0),
            "Timed out while sending navigation goal",
        )

        goal_handle = send_future.result()
        self.assertIsNotNone(goal_handle)
        self.assertTrue(goal_handle.accepted, "Nav2 rejected fixture goal")

        result_future = goal_handle.get_result_async()
        self.assertTrue(
            self._spin_until(lambda: result_future.done(), 120.0),
            "Deterministic fixture did not finish within 120 seconds",
        )

        result_response = result_future.result()
        self.assertEqual(
            result_response.status,
            GoalStatus.STATUS_SUCCEEDED,
            "Nav2 did not report successful goal completion",
        )

        self.assertGreater(
            len(self.trajectory),
            20,
            "Insufficient odometry samples to verify the run",
        )

        for x, y in self.trajectory:
            inside_expanded_obstacle = (
                FORBIDDEN_X[0] <= x <= FORBIDDEN_X[1]
                and FORBIDDEN_Y[0] <= y <= FORBIDDEN_Y[1]
            )
            self.assertFalse(
                inside_expanded_obstacle,
                f"Robot center entered expanded obstacle at ({x:.3f}, {y:.3f})",
            )

        final_x, final_y = self.trajectory[-1]
        final_error = math.hypot(final_x - GOAL_X, final_y - GOAL_Y)
        self.assertLessEqual(
            final_error,
            0.20,
            f"Final position error {final_error:.3f} m exceeds fixture tolerance",
        )

        # A straight line from start to goal intersects the central obstacle,
        # so a successful collision-free run must exhibit non-trivial detour.
        max_detour = max(abs(y) for _, y in self.trajectory)
        self.assertGreater(
            max_detour,
            1.05,
            "Trajectory did not demonstrate the expected obstacle detour",
        )
