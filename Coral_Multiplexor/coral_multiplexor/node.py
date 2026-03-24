from __future__ import annotations

from crab_client.events import DebugEvent, ReactionEvent, TicketState
from crab_client.types import CommandItem, HealthMetric, TicketType

from Coral_Multiplexor import DEFAULT_BRIDGE_PATH

from .serial_transport import SerialTransport, SerialTransportError
from .service_backend import CoralMultiplexorBackend

try:  # pragma: no cover
    import rclpy
    from rclpy.node import Node
except ImportError:  # pragma: no cover
    rclpy = None
    Node = object  # type: ignore[misc,assignment]


class CoralMultiplexorNode(Node):  # pragma: no cover
    def __init__(self) -> None:
        if rclpy is None:
            raise RuntimeError("rclpy is required to run the ROS2 multiplexor node")
        super().__init__("coral_multiplexor")
        bridge_path = self.declare_parameter(
            "bridge_path",
            str(DEFAULT_BRIDGE_PATH),
        ).value
        transport = None
        try:
            transport = SerialTransport(debug=bool(self.declare_parameter("debug_serial", False).value))
        except SerialTransportError as exc:
            self.get_logger().warning(f"Serial transport unavailable: {exc}")
        self.backend = CoralMultiplexorBackend(bridge_path=bridge_path, transport=transport)
        self.get_logger().info(f"Multiplexor backend initialized with status: {self.backend.get_bridge_status(self.backend.register_client('__bootstrap__')).message}")


def main(args: list[str] | None = None) -> None:  # pragma: no cover
    if rclpy is None:
        raise RuntimeError("rclpy is not installed")
    rclpy.init(args=args)
    node = CoralMultiplexorNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()
