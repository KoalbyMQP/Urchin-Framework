"""ROS-style log formatting for applications using the Crab API.

    [DEBUG] [1793300000.123] [crab-api2.link]: Sent VPID 0: b'Validate'

The library never installs handlers itself; applications opt in with ``setup_logging``.
"""

from __future__ import annotations

import logging
import sys

_PACKAGE = "crab_api2"
_DISPLAY_NAME = "crab-api2"

# ROS severity names
_LEVEL_LABELS = {
    logging.DEBUG: "DEBUG",
    logging.INFO: "INFO",
    logging.WARNING: "WARN",
    logging.ERROR: "ERROR",
    logging.CRITICAL: "FATAL",
}


class RosFormatter(logging.Formatter):
    """Formats records as ``[LEVEL] [unix_seconds.millis] [name]: message``."""

    def format(self, record: logging.LogRecord) -> str:
        level = _LEVEL_LABELS.get(record.levelno, record.levelname)
        millis = int(record.created * 1000)
        name = record.name
        if name == _PACKAGE or name.startswith(_PACKAGE + "."):
            name = _DISPLAY_NAME + name[len(_PACKAGE) :]
        # The base class adds exception and stack text after the message
        message = super().format(record)
        return f"[{level}] [{millis // 1000}.{millis % 1000:03d}] [{name}]: {message}"


def setup_logging(level: int = logging.INFO) -> None:
    """Log to stderr in ROS style at ``level``. Safe to call more than once."""
    root = logging.getLogger()
    root.setLevel(level)
    if any(isinstance(h.formatter, RosFormatter) for h in root.handlers):
        return
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(RosFormatter())
    root.addHandler(handler)
