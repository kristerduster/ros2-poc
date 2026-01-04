#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class AlsoSubscriber(Node):
    def __init__(self) -> None:
        super().__init__('also_subscriber_node')
        self.subscription = self.create_subscription(
            String,
            'communication_topic',
            self._callback,
            10,
        )

    def _callback(self, msg: String) -> None:
        # Mirror C++ subscriber logging, but distinguish this node
        self.get_logger().info(f"I also heard: '{msg.data}'")


def main(args=None) -> None:
    rclpy.init(args=args)
    node = AlsoSubscriber()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
