# Changelog

## PR: Polyglot ROS2 pub/sub string demo
- [src/sub_pub/src/publisher.cpp](src/sub_pub/src/publisher.cpp): C++ publisher `publisher_node` emits sequential strings like `Message Number: 1` every second on `communication_topic`.
- [src/sub_pub/src/subscriber.cpp](src/sub_pub/src/subscriber.cpp): C++ subscriber logs incoming originals from `communication_topic`, e.g., `I heard: 'Message Number: 1'`.
- [src/sub_pub/scripts/also_subscriber.py](src/sub_pub/scripts/also_subscriber.py): Python subscriber mirrors the log style, e.g., `I also heard: 'Message Number: 1'` from `communication_topic`.
- [src/sub_pub/src/edited_subscriber.cpp](src/sub_pub/src/edited_subscriber.cpp): C++ node listens on `communication_topic`, appends ` edit`, publishes `Message Number: 1 edit` to `edited_topic`, and logs both.
- [src/sub_pub/src/edited_listener.cpp](src/sub_pub/src/edited_listener.cpp): C++ listener for `edited_topic`, logging edited payloads like `Edited listener heard: 'Message Number: 1 edit'`.
- [src/sub_pub/CMakeLists.txt](src/sub_pub/CMakeLists.txt): Registers and installs all C++ and Python nodes with `ament_cmake`.
- [src/sub_pub/package.xml](src/sub_pub/package.xml): Declares package metadata and dependencies, including `rclcpp`, `std_msgs`, and `rclpy` for the Python node.
