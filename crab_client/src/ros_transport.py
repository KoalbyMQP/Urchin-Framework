from __future__ import annotations

from collections.abc import Callable

from .events import BridgeStatus, DebugEvent, ReactionEvent, TicketState
from .types import CommandItem, HealthMetric, TicketType

try:  # pragma: no cover
    import rclpy
    from rclpy.executors import SingleThreadedExecutor
    from rclpy.node import Node
except ImportError:  # pragma: no cover
    rclpy = None
    Node = object  # type: ignore[misc,assignment]
    SingleThreadedExecutor = object  # type: ignore[misc,assignment]


class RosTransportError(RuntimeError):
    pass


class RosTransport:
    def __init__(self, node_name: str = "crab_client") -> None:
        if rclpy is None:
            raise RosTransportError("rclpy is not installed")
        self._node_name = node_name
        self._session_id: int | None = None
        self._node: Node | None = None
        self._executor: SingleThreadedExecutor | None = None
        self._debug_handlers: list[tuple[int, Callable[[DebugEvent], None]]] = []
        self._ticket_handlers: list[tuple[int, Callable[[TicketState], None]]] = []
        self._reaction_handlers: list[tuple[int, Callable[[ReactionEvent], None]]] = []
        if not rclpy.ok():
            rclpy.init()
        self._node = rclpy.create_node(self._node_name)
        self._executor = SingleThreadedExecutor()
        self._executor.add_node(self._node)

    def register_client(self, client_name: str) -> int:
        self._session_id = 1
        return self._session_id

    def unregister_client(self, session_id: int) -> None:
        self._session_id = None

    def get_bridge_status(self, session_id: int) -> BridgeStatus:
        raise RosTransportError("ROS service bindings are not available until the package is built with rosidl")

    def submit_ticket(self, session_id: int, ticket_type: TicketType, items: list[CommandItem], chained: bool) -> int:
        raise RosTransportError("ROS service bindings are not available until the package is built with rosidl")

    def close_ticket(self, session_id: int, ticket_id: int) -> None:
        raise RosTransportError("ROS service bindings are not available until the package is built with rosidl")

    def get_ticket_info(self, session_id: int, ticket_id: int) -> TicketState:
        raise RosTransportError("ROS service bindings are not available until the package is built with rosidl")

    def get_health(self, session_id: int, metric: HealthMetric) -> str:
        raise RosTransportError("ROS service bindings are not available until the package is built with rosidl")

    def subscribe_debug(self, session_id: int, handler: Callable[[DebugEvent], None]) -> None:
        self._debug_handlers.append((session_id, handler))

    def subscribe_ticket_state(self, session_id: int, handler: Callable[[TicketState], None]) -> None:
        self._ticket_handlers.append((session_id, handler))

    def subscribe_reactions(self, session_id: int, handler: Callable[[ReactionEvent], None]) -> None:
        self._reaction_handlers.append((session_id, handler))

    def close(self) -> None:
        if self._executor is not None and self._node is not None:
            self._executor.remove_node(self._node)
        if self._node is not None:
            self._node.destroy_node()
        self._node = None
        self._executor = None
