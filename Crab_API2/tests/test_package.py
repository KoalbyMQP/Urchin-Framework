import crab_api2
from crab_api2 import bridge, crab, herkulex, link, reaction


def test_public_exports():
    assert set(crab_api2.__all__) == {
        "Crab",
        "TicketType",
        "Reaction",
        "ReactionEvent",
        "herkulex",
        "load_bridge",
        "CrabError",
        "NoFreeTicketError",
        "ReplyTimeoutError",
        "UnknownJointError",
        "BridgeError",
        "LinkError",
        "ReactionError",
    }


def test_exports_are_the_real_objects():
    assert crab_api2.Crab is crab.Crab
    assert crab_api2.TicketType is crab.TicketType
    assert crab_api2.Reaction is reaction.Reaction
    assert crab_api2.ReactionEvent is reaction.ReactionEvent
    assert crab_api2.herkulex is herkulex
    assert crab_api2.load_bridge is bridge.load_bridge
    assert crab_api2.BridgeError is bridge.BridgeError
    assert crab_api2.LinkError is link.LinkError
    assert crab_api2.ReactionError is reaction.ReactionError


def test_all_names_resolve():
    for name in crab_api2.__all__:
        assert getattr(crab_api2, name) is not None
