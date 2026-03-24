Coral Multiplexor is now the ROS2 boundary for Urchin hardware access.

Responsibilities:
- own serial/UART access to firmware
- load and upload the canonical bridge TOML
- expose multi-client request/response APIs over ROS2
- fan out debug, reaction, exchange, and ticket-state events

Repo-local Python imports use `Coral_Multiplexor` because this repository currently lives on a case-insensitive filesystem.
The ROS2 package metadata still uses the lowercase package name `coral_multiplexor`.
The Python client is its own codebase under `crab_client/` with a distributable `src/` layout.
