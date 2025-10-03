import sigrokdecode as srd

class Decoder(srd.Decoder):
    api_version = 3
    id = 'herkulex'
    name = 'HerkuleX'
    longname = 'HerkuleX'
    desc = 'HerkuleX Decoder'
    license = 'gplv2+'
    inputs = ['uart']      # stack on the built-in UART decoder
    outputs = []

    Index: int = 0

    #Herkulex packet data
    Header: int = 0xFF
    Len: int = 0
    Command: int = 0
    Servo: int = 0
    Checksum1: int = 0
    Checksum2: int = 0
    PacketData = []

    commands = {
        0x01: 'EEP_WRITE',
        0x02: 'EEP_READ',
        0x03: 'RAM_WRITE',
        0x04: 'RAM_READ',
        0x05: 'I_JOG',
        0x06: 'S_JOG',
        0x07: 'STAT',
        0x08: 'ROLLBACK',
        0x09: 'REBOOT'
    }


    # Define annotation types (id, description)
    annotations = (
        ('data', 'Data'),            # index 0

    )

    # Combine annotation types into visible rows in the UI.
    # Tuple form: (row_id, row_label, (annotation_indexes,...))
    annotation_rows = (
        ('data', 'Data', (0,)),      # row showing data annotations (ann index 0)

    )

    def __init__(self):
        # any state init goes here
        self.reset()

    def reset(self):
        pass

    def start(self):
        # register annotation output so we can post GUI annotations
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def decode_byte(int)->bool:
        """
        Called for each byte in the packet
        :return: True if this would cause a packet error
        """



    def check_header(byte: int) -> bool:
        """Return True if this byte is a HerkuleX header byte (0xFF)."""
        return byte == 0xFF


    def decode(self, ss, es, data):
        """
        ss, es: start/end sample numbers provided by stacked UART decoder
        data: packets forwarded by the UART decoder (e.g. 'DATA', 'FRAME', etc.)
        """
        if not data:
            return
        if not isinstance(data, (list, tuple)) or len(data) < 1:
            return

        ptype = data[0]

        # DATA: ['DATA', rxtx, (value, databits_list)]

        if ptype == 'STARTBIT':
            self.Index=0
            self.PacketData=[]



        if ptype == 'DATA':
            try:
                payload = data[2]
                val = payload[0] if isinstance(payload, (list, tuple)) else int(payload)
            except Exception:
                return
            long_text = f"Data: 0x{val:02X}"
            mid_text = f"D:0x{val:02X}"
            short_text = f"{val:02X}"
            self.put(ss, es, self.out_ann, [0, [long_text, mid_text,short_text]])
            return

        # FRAME: ['FRAME', rxtx, (value, valid)]
        if ptype == 'FRAME':
            try:
                val, valid = data[2]
            except Exception:
                return
            if valid:
                self.put(ss, es, self.out_ann, [0, [f"Frame: 0x{val:02X}", f"{val:02X}"]])
            else:
                self.put(ss, es, self.out_ann, [0, [f"Frame error: 0x{val:02X}", "Frame err"]])
            return

        # PARITY ERROR: ['PARITY ERROR', rxtx, (expected, actual)]
        if ptype == 'PARITY ERROR':
            try:
                expected, actual = data[2]
            except Exception:
                return
            self.put(ss, es, self.out_ann, [0, [f"Parity error exp={expected} act={actual}", "Parity err"]])
            return

        # INVALID STARTBIT / STOPBIT
        if ptype == 'INVALID STARTBIT':
            try:
                val = data[2]
            except Exception:
                val = None
            self.put(ss, es, self.out_ann, [0, [f"Invalid start bit: {val}", "Start err"]])
            return

        if ptype == 'INVALID STOPBIT':
            try:
                val = data[2]
            except Exception:
                val = None
            self.put(ss, es, self.out_ann, [0, [f"Invalid stop bit: {val}", "Stop err"]])
            return

        # BREAK
        if ptype == 'BREAK':
            self.put(ss, es, self.out_ann, [0, ["Break condition", "Break"]])
            return

        # IDLE
        if ptype == 'IDLE':
            self.put(ss, es, self.out_ann, [0, ["Idle", "Idle"]])
            return

        # fallback (debug)
        # self.put(ss, es, self.out_ann, [0, [f"Pkt:{ptype}", str(data)]])