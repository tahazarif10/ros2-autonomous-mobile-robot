from pathlib import Path
import xml.etree.ElementTree as ET

import xacro


MODEL = Path(__file__).parents[1] / "urdf" / "amr.urdf.xacro"

EXPECTED_LINKS = {
    "base_link",
    "left_wheel_link",
    "right_wheel_link",
    "caster_link",
}

EXPECTED_JOINTS = {
    "left_wheel_joint": "continuous",
    "right_wheel_joint": "continuous",
    "caster_joint": "fixed",
}


def expanded_robot():
    document = xacro.process_file(str(MODEL))
    return ET.fromstring(document.toxml())


def test_xacro_expands_to_expected_robot():
    robot = expanded_robot()

    assert robot.tag == "robot"
    assert robot.attrib["name"] == "amr"

    links = {link.attrib["name"] for link in robot.findall("link")}
    assert EXPECTED_LINKS <= links

    joints = {
        joint.attrib["name"]: joint.attrib["type"]
        for joint in robot.findall("joint")
    }
    assert EXPECTED_JOINTS.items() <= joints.items()


def test_all_joint_links_exist():
    robot = expanded_robot()
    links = {link.attrib["name"] for link in robot.findall("link")}

    for joint in robot.findall("joint"):
        parent = joint.find("parent")
        child = joint.find("child")

        assert parent is not None
        assert child is not None
        assert parent.attrib["link"] in links
        assert child.attrib["link"] in links


def test_drive_wheels_have_expected_rotation_axis():
    robot = expanded_robot()

    for name in ("left_wheel_joint", "right_wheel_joint"):
        joint = robot.find(f"./joint[@name='{name}']")
        assert joint is not None

        axis = joint.find("axis")
        assert axis is not None
        assert axis.attrib["xyz"] == "0 1 0"
