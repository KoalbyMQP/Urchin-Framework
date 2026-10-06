from src.dbox import Decoder, crc16, encode

# Golden frames produced by Clam's C++ D-BOX code (DBOXraw, from the vendored
# copy in Urchin_Firmware/lib/D-BOX-vendored) -- the Python encoder must stay
# byte-identical to these.
GOLDEN = [
    ((0, 'S', "Urchin"), "0700530600524c55726368696e"),
    ((7, 'T', "Testing a different payload!"),
     "0707541c006a0754657374696e67206120646966666572656e74207061796c6f616421"),
    ((0, 'N', "Spin -200"), "07004e0900069c5370696e202d323030"),
    ((255, 'E', ""), "07ff450000e03c"),
    ((3, 'D', "x" * 300), "0703442c01b25f" + "78" * 300),
]


def test_dbox_encode_matches_cpp():
    for args, want in GOLDEN:
        assert encode(*args).hex() == want


def test_dbox_crc16_header():
    assert crc16(bytes([0, ord('S'), 6, 0])) == 0x4C52


def test_dbox_decoder_resyncs_past_noise_and_false_starts():
    stream = b"ets Jun  8 2016 boot\r\n\x07\x01junk" + b"".join(bytes.fromhex(w) for _, w in GOLDEN)
    decoder = Decoder()
    decoded = []
    for byte in stream:   # worst case: bytes trickle in one at a time
        decoder.feed(bytes([byte]))
        packet = decoder.next_packet()
        while packet is not None:
            decoded.append((packet.vpid, packet.stream, packet.text))
            packet = decoder.next_packet()
    assert decoded == [args for args, _ in GOLDEN]


def test_dbox_decoder_waits_for_partial_frame():
    frame = encode(0, 'E', "OK STOP")
    decoder = Decoder()
    decoder.feed(frame[:-2])
    assert decoder.next_packet() is None
    decoder.feed(frame[-2:])
    assert decoder.next_packet().text == "OK STOP"
