import logging

import pytest

from crab_api2.log import RosFormatter, setup_logging


def make_record(name: str, level: int, msg: str, created: float = 1793300000.123456):
    record = logging.LogRecord(name, level, __file__, 1, msg, None, None)
    record.created = created
    return record


@pytest.mark.parametrize(
    ("level", "label"),
    [
        (logging.DEBUG, "DEBUG"),
        (logging.INFO, "INFO"),
        (logging.WARNING, "WARN"),
        (logging.ERROR, "ERROR"),
        (logging.CRITICAL, "FATAL"),
    ],
)
def test_level_labels(level, label):
    line = RosFormatter().format(make_record("crab_api2.link", level, "hi"))
    assert line.startswith(f"[{label}] ")


def test_format_layout():
    record = make_record("crab_api2.link", logging.DEBUG, "Sent VPID 0: b'Validate'")
    assert RosFormatter().format(record) == (
        "[DEBUG] [1793300000.123] [crab-api2.link]: Sent VPID 0: b'Validate'"
    )


def test_timestamp_truncates_to_milliseconds():
    record = make_record("crab_api2", logging.INFO, "x", created=1793300000.9999)
    assert "[1793300000.999]" in RosFormatter().format(record)


def test_package_root_name():
    line = RosFormatter().format(make_record("crab_api2", logging.INFO, "x"))
    assert "[crab-api2]:" in line


def test_firmware_logger_name():
    line = RosFormatter().format(make_record("crab_api2.firmware", logging.DEBUG, "x"))
    assert "[crab-api2.firmware]:" in line


def test_other_logger_names_unchanged():
    line = RosFormatter().format(make_record("serial.urlhandler", logging.INFO, "x"))
    assert "[serial.urlhandler]:" in line


def test_message_args_and_traceback():
    try:
        raise RuntimeError("boom")
    except RuntimeError:
        import sys

        record = logging.LogRecord(
            "crab_api2.crab",
            logging.ERROR,
            __file__,
            1,
            "failed %d",
            (3,),
            sys.exc_info(),
        )
    line = RosFormatter().format(record)
    assert line.splitlines()[0].endswith("[crab-api2.crab]: failed 3")
    assert "RuntimeError: boom" in line


def test_setup_logging(capsys):
    root = logging.getLogger()
    saved = root.handlers[:], root.level
    root.handlers.clear()
    try:
        setup_logging(logging.DEBUG)
        setup_logging(logging.DEBUG)  # calling twice must not duplicate output
        logging.getLogger("crab_api2.link").debug("hello")
        err = capsys.readouterr().err
        assert err.count("[crab-api2.link]: hello") == 1
        assert err.startswith("[DEBUG] [")
    finally:
        root.handlers[:], level = saved
        root.setLevel(level)
