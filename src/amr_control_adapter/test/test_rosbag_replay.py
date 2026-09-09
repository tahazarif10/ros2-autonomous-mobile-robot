import json
import math
from pathlib import Path
import tempfile
import time
import unittest

from diagnostic_msgs.msg import DiagnosticArray
from geometry_msgs.msg import Twist
import launch
import launch_testing
from launch_ros.actions import Node
from lifecycle_msgs.msg import Transition
from lifecycle_msgs.srv import ChangeState
from nav_msgs.msg import Odometry, Path as PathMessage
from geometry_msgs.msg import PoseStamped
import pytest
import rclpy
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from rclpy.serialization import deserialize_message, serialize_message
import rosbag2_py


@pytest.mark.launch_test
def generate_test_description():
    adapter = Node(
        package="amr_control_adapter",
        executable="amr_control_adapter_node",
        name="amr_control_adapter",
        namespace="replay",
        output="screen",
        parameters=[
            {
                "control_rate_hz": 50.0,
                "stale_timeout_s": 2.0,
            }
        ],
    )

    return (
        launch.LaunchDescription(
            [
                adapter,
                launch_testing.actions.ReadyToTest(),
            ]
        ),
        {"adapter": adapter},
    )


class TestRosbagReplayRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def setUp(self):
        self.node = rclpy.create_node(
            "rosbag_replay_regression",
            namespace="/replay",
        )
        self.change_state = self.node.create_client(
            ChangeState,
            "/replay/amr_control_adapter/change_state",
        )

        path_qos = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        self.path_pub = self.node.create_publisher(
            PathMessage,
            "plan",
            path_qos,
        )
        odom_qos = QoSProfile(
            depth=10,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.VOLATILE,
        )
        self.odom_pub = self.node.create_publisher(
            Odometry,
            "odom",
            odom_qos,
        )

        self.cmd_sub = self.node.create_subscription(
            Twist,
            "cmd_vel",
            self._on_cmd,
            10,
        )
        self.diag_sub = self.node.create_subscription(
            DiagnosticArray,
            "diagnostics",
            self._on_diag,
            10,
        )

        self.max_linear = 0.0
        self.latest_command = Twist()
        self.reasons = []
        self.command_samples = 0
        self.diagnostic_samples = 0

        fixture_path = Path(__file__).parent / "fixtures" / "control_replay.json"
        with fixture_path.open("r", encoding="utf-8") as stream:
            self.fixture = json.load(stream)

        self.temp_dir = tempfile.TemporaryDirectory(prefix="amr_replay_")
        self.bag_uri = str(Path(self.temp_dir.name) / "control_fixture")
        self._write_bag()

        self.assertTrue(
            self.change_state.wait_for_service(timeout_sec=20.0),
            "Lifecycle change_state service did not become available",
        )
        self._transition(Transition.TRANSITION_CONFIGURE)
        self._transition(Transition.TRANSITION_ACTIVATE)

        self.assertTrue(
            self._spin_until(
                lambda: (
                    self.path_pub.get_subscription_count() > 0
                    and self.odom_pub.get_subscription_count() > 0
                ),
                10.0,
            ),
            "Adapter subscriptions were not discovered",
        )

    def tearDown(self):
        try:
            self._transition(Transition.TRANSITION_DEACTIVATE, required=False)
            self._transition(Transition.TRANSITION_CLEANUP, required=False)
        finally:
            self.node.destroy_subscription(self.diag_sub)
            self.node.destroy_subscription(self.cmd_sub)
            self.node.destroy_publisher(self.odom_pub)
            self.node.destroy_publisher(self.path_pub)
            self.node.destroy_client(self.change_state)
            self.node.destroy_node()
            self.temp_dir.cleanup()

    def _on_cmd(self, message):
        self.latest_command = message
        self.max_linear = max(self.max_linear, abs(message.linear.x))
        self.command_samples += 1

    def _on_diag(self, message):
        if message.status:
            self.reasons.append(message.status[0].message)
            self.diagnostic_samples += 1

    def _transition(self, transition_id, required=True):
        request = ChangeState.Request()
        request.transition.id = transition_id
        future = self.change_state.call_async(request)

        if not self._spin_until(lambda: future.done(), 10.0):
            if required:
                self.fail(f"Lifecycle transition {transition_id} timed out")
            return False

        response = future.result()
        success = response is not None and response.success
        if required:
            self.assertTrue(success, f"Lifecycle transition {transition_id} failed")
        return success

    def _spin_for(self, duration_s):
        deadline = time.monotonic() + duration_s
        while time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            rclpy.spin_once(self.node, timeout_sec=min(0.02, max(0.0, remaining)))

    def _spin_until(self, predicate, timeout_s):
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            rclpy.spin_once(self.node, timeout_sec=0.02)
            if predicate():
                return True
        return predicate()

    def _make_path(self):
        message = PathMessage()
        message.header.frame_id = "map"
        for x, y in self.fixture["path"]:
            pose = PoseStamped()
            pose.header.frame_id = "map"
            pose.pose.position.x = float(x)
            pose.pose.position.y = float(y)
            pose.pose.orientation.w = 1.0
            message.poses.append(pose)
        return message

    @staticmethod
    def _make_odom(x):
        message = Odometry()
        message.header.frame_id = "odom"
        message.child_frame_id = "base_link"
        message.pose.pose.position.x = float(x)
        message.pose.pose.orientation.w = 1.0
        return message

    def _write_bag(self):
        writer = rosbag2_py.SequentialWriter()
        writer.open(
            rosbag2_py.StorageOptions(
                uri=self.bag_uri,
                storage_id="sqlite3",
            ),
            rosbag2_py.ConverterOptions("", ""),
        )

        writer.create_topic(
            rosbag2_py.TopicMetadata(
                id=0,
                name="/plan",
                type="nav_msgs/msg/Path",
                serialization_format="cdr",
            )
        )
        writer.create_topic(
            rosbag2_py.TopicMetadata(
                id=1,
                name="/odom",
                type="nav_msgs/msg/Odometry",
                serialization_format="cdr",
            )
        )

        base_ns = 1_000_000_000
        path_message = self._make_path()

        for event in self.fixture["events"]:
            timestamp = base_ns + int(event["time_ns"])
            if event["topic"] == "/plan":
                message = path_message
            elif event["topic"] == "/odom":
                message = self._make_odom(event["x"])
            else:
                raise AssertionError(f"Unsupported fixture topic: {event['topic']}")

            writer.write(
                event["topic"],
                serialize_message(message),
                timestamp,
            )

        writer.close()

    def _reset_observations(self):
        self.max_linear = 0.0
        self.latest_command = Twist()
        self.reasons = []
        self.command_samples = 0
        self.diagnostic_samples = 0

    def _replay_once(self):
        self._reset_observations()

        reader = rosbag2_py.SequentialReader()
        reader.open(
            rosbag2_py.StorageOptions(
                uri=self.bag_uri,
                storage_id="sqlite3",
            ),
            rosbag2_py.ConverterOptions("", ""),
        )

        first_timestamp = None
        last_timestamp = None
        message_count = 0
        speed = float(self.fixture["replay_speed"])
        replay_started = time.monotonic()

        while reader.has_next():
            topic, data, timestamp = reader.read_next()

            if first_timestamp is None:
                first_timestamp = timestamp
            message_count += 1

            if last_timestamp is not None:
                delta_s = (timestamp - last_timestamp) / 1_000_000_000.0
                self._spin_for(delta_s / speed)
            last_timestamp = timestamp

            if topic == "/plan":
                self.path_pub.publish(deserialize_message(data, PathMessage))
            elif topic == "/odom":
                self.odom_pub.publish(deserialize_message(data, Odometry))
            else:
                self.fail(f"Unexpected topic in replay bag: {topic}")

            self._spin_for(0.03)

        replay_elapsed_s = time.monotonic() - replay_started
        del reader

        self.assertTrue(
            self._spin_until(lambda: "goal_reached" in self.reasons, 2.0),
            "Replay never reached the goal",
        )

        final_zero = (
            math.isclose(self.latest_command.linear.x, 0.0, abs_tol=1.0e-9)
            and math.isclose(self.latest_command.angular.z, 0.0, abs_tol=1.0e-9)
        )

        bag_span_s = (last_timestamp - first_timestamp) / 1_000_000_000.0

        return {
            "outcome": {
                "saw_motion": self.max_linear > 0.01,
                "saw_normal": "none" in self.reasons,
                "saw_goal": "goal_reached" in self.reasons,
                "final_zero": final_zero,
            },
            "metrics": {
                "bag_message_count": message_count,
                "bag_span_s": bag_span_s,
                "replay_elapsed_s": replay_elapsed_s,
                "max_linear_mps": self.max_linear,
                "command_samples": self.command_samples,
                "diagnostic_samples": self.diagnostic_samples,
            },
        }

    def _assert_metrics(self, replay_result):
        metrics = replay_result["metrics"]
        expected_span_s = (
            self.fixture["events"][-1]["time_ns"]
            - self.fixture["events"][0]["time_ns"]
        ) / 1_000_000_000.0
        nominal_paced_s = expected_span_s / float(self.fixture["replay_speed"])

        self.assertEqual(
            metrics["bag_message_count"],
            len(self.fixture["events"]),
        )
        self.assertAlmostEqual(metrics["bag_span_s"], expected_span_s, places=9)
        self.assertGreaterEqual(
            metrics["replay_elapsed_s"],
            nominal_paced_s * 0.8,
        )
        self.assertLess(
            metrics["replay_elapsed_s"],
            nominal_paced_s + 2.0,
        )
        self.assertGreater(metrics["max_linear_mps"], 0.01)
        self.assertLessEqual(metrics["max_linear_mps"], 0.85 + 1.0e-9)
        self.assertGreater(metrics["command_samples"], 5)
        self.assertGreater(metrics["diagnostic_samples"], 5)

    def test_same_rosbag_replays_to_same_control_outcome(self, adapter):
        first = self._replay_once()
        second = self._replay_once()

        for result in (first, second):
            self.assertTrue(result["outcome"]["saw_motion"])
            self.assertTrue(result["outcome"]["saw_normal"])
            self.assertTrue(result["outcome"]["saw_goal"])
            self.assertTrue(result["outcome"]["final_zero"])
            self._assert_metrics(result)

        self.assertEqual(first["outcome"], second["outcome"])

