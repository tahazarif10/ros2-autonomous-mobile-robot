import subprocess
import time
import unittest

import launch
import launch_testing
import pytest
from launch.actions import ExecuteProcess


@pytest.mark.launch_test
def generate_test_description():
    display_launch = ExecuteProcess(
        cmd=[
            "ros2",
            "launch",
            "amr_bringup",
            "display.launch.py",
            "use_rviz:=false",
        ],
        output="screen",
    )

    return (
        launch.LaunchDescription(
            [
                display_launch,
                launch_testing.actions.ReadyToTest(),
            ]
        ),
        {"display_launch": display_launch},
    )


class TestDisplayLaunch(unittest.TestCase):
    def test_required_nodes_are_running(self, proc_info, display_launch):
        proc_info.assertWaitForStartup(process=display_launch, timeout=10)

        expected = {
            "/joint_state_publisher",
            "/robot_state_publisher",
        }
        deadline = time.monotonic() + 15.0
        last_output = ""

        while time.monotonic() < deadline:
            result = subprocess.run(
                ["ros2", "node", "list"],
                check=False,
                capture_output=True,
                text=True,
                timeout=5,
            )
            last_output = result.stdout
            nodes = {
                line.strip()
                for line in result.stdout.splitlines()
                if line.strip()
            }

            if result.returncode == 0 and expected <= nodes:
                return

            time.sleep(0.5)

        self.fail(
            "Required ROS 2 nodes did not become available. "
            f"Expected {sorted(expected)}, last node list: {last_output!r}"
        )
