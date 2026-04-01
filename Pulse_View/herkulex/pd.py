import sigrokdecode as srd
from . import Memory

#from typing import List, Optional


class Sample():
    def __init__(self,data, start, end):
        self.data = data
        self.start = start
        self.end = end




class Decoder(srd.Decoder):
    api_version = 3
    id = 'herkulex'
    name = 'HerkuleX'
    longname = 'HerkuleX'
    desc = 'HerkuleX Servo Protocol Decoder'
    license = 'gplv2+'
    inputs = ['uart']      # stack on top of UART
    outputs = []
    tags = ['Remote Control', 'Embedded/industrial', 'Debug/trace']

    MIN_SAMPLERATE = 2000000

    DataIndex = 0
    FrameIndex = 0

    # Herkulex packet data
    Header = 0xFF

    #Record----
    RecordData = []
    RecordLength = 0
    RecordIndex = 0

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



    MotorMemory = []

    def __init__(self, *args, **kwargs):
        super().__init__()




    # Annotation definitions
    annotations = (
        ('herkulex', 'HerkuleX data'),  # index 0
        ('invalid', 'Invalid/Error packet'),  # index 1
    )

    annotation_rows = (
        ('data_row', 'Data', (0,)),  # row for normal packets
        ('error_row', 'Validate', (1,)),  # row for invalid/error packets
    )

    def __init__(self):
        self.reset()

    def reset(self):

        self.RecordData = []
        self.RecordLength = 0
        self.RecordIndex = 0

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)
        self.MotorMemory = [
            Memory.Memory("Model No1", 1, 0, None, None, None, "RO", None),
            Memory.Memory("Model No2", 1, 1, None, None, None, "RO", None),
            Memory.Memory("Firmware Version 1/2", 1, 2, None, None, None, "RO", None),
            Memory.Memory("Firmware Version 2/2", 1, 3, None, None, None, "RO", None),
            Memory.Memory("Baud Rate", 1, 4, None, None, None, "RW", None),
            Memory.Memory("ID", 1, 6, 0, 0x00, 0xFD, "RW", None),
            Memory.Memory("ACK Policy", 1, 7, 1, 0x00, 0x20, "RW", self.ACK),
            Memory.Memory("Alarm LED Policy", 1, 8, 2, 0x00, 0x7F, "RW", None),
            Memory.Memory("Torque Policy", 1, 9, 3, 0x00, 0x7F, "RW", None),
            Memory.Memory("Max Temperature", 1, 11, 5, 0, 110, "RW", None),
            Memory.Memory("Min Voltage", 1, 12, 6, 92, 200, "RW", None),
            Memory.Memory("Max Voltage", 1, 13, 7, 92, 200, "RW", None),
            Memory.Memory("Acceleration Ratio", 1, 14, 8, 0x00, 50, "RW", None),
            Memory.Memory("Max Acceleration Time", 1, 15, 9, 0x00, 0xFE, "RW", None),
            Memory.Memory("Dead Zone", 1, 16, 10, 0x00, 0xFE, "RW", None),
            Memory.Memory("Saturator Offset", 1, 17, 11, 0x00, 0xFE, "RW", None),
            Memory.Memory("Saturator Slope", 2, 18, 12, 0x0000, 0x7FFF, "RW", None),
            Memory.Memory("PWM Offset", 1, 20, 14, -128, 127, "RW", None),
            Memory.Memory("Min PWM", 1, 21, 15, 0, 254, "RW", None),
            Memory.Memory("Max PWM", 2, 22, 16, 0x0000, 0x03FF, "RW", None),
            Memory.Memory("Overload PWM Threshold", 2, 18, 18, 0x0000, 0x03FF, "RW", None),
            Memory.Memory("Min Position", 2, 26, 20, 0, 2047, "RW", None),
            Memory.Memory("Max Position", 2, 28, 22, 0, 2047, "RW", None),
            Memory.Memory("Position Kp", 2, 30, 24, 0x0000, 0x7FFF, "RW", None),
            Memory.Memory("Position Kd", 2, 32, 26, 0x0000, 0x7FFF, "RW", None),
            Memory.Memory("Position Ki", 2, 34, 28, 0x0000, 0x7FFF, "RW", None),
            Memory.Memory("Position Feedforward Gain 1", 2, 36, 30, 0x0000, 0x7FFF, "RW", None),
            Memory.Memory("Position Feedforward Gain 2", 2, 38, 32, 0x0000, 0x7FFF, "RW", None),
            Memory.Memory("LED Blink Period", 1, 44, 38, 0x00, 0xFE, "RW", None),
            Memory.Memory("ADC Fault Check Period", 1, 45, 39, 0x00, 0xFE, "RW", None),
            Memory.Memory("Packet Garbage Check Period", 1, 46, 40, 0x00, 0xFE, "RW", None),
            Memory.Memory("Stop Detection Period", 1, 47, 41, 0x00, 0xFE, "RW", None),
            Memory.Memory("Overload Detection Period", 1, 48, 42, 0x00, 0xFE, "RW", None),
            Memory.Memory("Stop Threshold", 1, 49, 43, 0x00, 0xFE, "RW", None),
            Memory.Memory("Inposition Margin", 1, 50, 44, 0x00, 0xFE, "RW", None),
            Memory.Memory("Calibration Difference Byte 1", 1, 52, 46, None, None, "RW", None),
            Memory.Memory("Calibration Difference Byte 2", 1, 53, 47, None, None, "RW", None),
            Memory.Memory("Status Error", 1, None, 48, 0x00, 0x7F, "RW", self.StatusError),
            Memory.Memory("Status Detail", 1, None, 49, 0x00, 0x7F, "RW", self.StatusDetail),
            Memory.Memory("Torque Control", 1, None, 52, None, None, "RW", self.TorqueControl),
            Memory.Memory("LED Control", 1, None, 53, 0x00, 0x07, "RW", None),
            Memory.Memory("Voltage", 1, None, 54, 0, 200, "RO", None),
            Memory.Memory("Temperature", 1, None, 55, 0, 110, "RO", None),
            Memory.Memory("Current Control Mode", 1, None, 56, 0, 1, "RO", None),
            Memory.Memory("Tick", 1, None, 57, 0x00, 0xFF, "RO", None),
            Memory.Memory("Calibrated Position", 2, None, 58, None, None, "RO", None),
            Memory.Memory("Absolute Position", 2, None, 60, None, None, "RO", None),
            Memory.Memory("Differential Position", 2, None, 62, None, None, "RO", None),
            Memory.Memory("PWM", 2, None, 64, None, None, "RO", None),
            Memory.Memory("Absolute Goal Position", 2, None, 68, None, None, "RO", None),
            Memory.Memory("Absolute Desired Trajectory Position", 2, None, 70, None, None, "RO", None),
            Memory.Memory("Desired Velocity", 2, None, 72, None, None, "RO", None),
        ]




    def Record(self, number, ss, es):
        # --- Header area (expect two 0xFF bytes) ---
        if self.RecordIndex < 2:
            if number == self.Header:
                self.RecordData.append(Sample(number, ss, es))
                self.RecordIndex += 1
            else:
                # reset and *re-check* current byte as potential first header byte
                self.reset()
                if number == self.Header:
                    self.RecordData.append(Sample(number, ss, es))
                    self.RecordIndex = 1
            return

        # --- Length byte (index == 2) ---
        if self.RecordIndex == 2:
            self.RecordData.append(Sample(number, ss, es))
            self.RecordIndex += 1

            # Basic validation: length must be at least header+length fields (e.g. 3 or more)
            # and must not be ridiculously large (adjust max_len to your protocol)
            max_len = 256
            if number < 3 or number > max_len:
                # invalid length -> drop and resync
                self.reset()
                return

            self.RecordLength = number  # total length (including header)
            # If length equals number of bytes we already have, decode immediately
            if len(self.RecordData) == self.RecordLength:
                self.decode_packet()
            return

        # --- Data bytes (including checksums and payload) ---
        # Append the byte
        self.RecordData.append(Sample(number, ss, es))
        self.RecordIndex += 1

        # If we've reached the expected total length, decode now (do it in the same call)
        if len(self.RecordData) == self.RecordLength:
            self.decode_packet()
        return

    def decode_packet(self):
        command_name = ""
        ACK = False
        for i in range(1,7):
            if i==1: #header
                start = self.RecordData[0].start
                end =  self.RecordData[i].end

                long_text = "Header %d" % len(self.RecordData)
                mid_text = "Head"
                short_text = "H"
                self.put(start, end, self.out_ann, [0, [long_text,mid_text,short_text]])

            if i==2:
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                long_text = "Length %d" % number
                mid_text = "Len %d" % number
                short_text = "%d" % number
                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

            if i==3:
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                if (number == 254):
                    long_text = "All Servos"
                    mid_text = "All"
                    short_text = "All"
                else:
                    long_text = "Servo ID %d" % number
                    mid_text = "ID %d" % number
                    short_text = "%d" % number
                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

            if i==4:
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                command_name = self.commands.get(number, "UNKNOWN")

                long_text = command_name
                mid_text = "%d" % number
                short_text = "%d" % number

                # Check if its an ACK Packet
                if (command_name == "UNKNOWN"):
                    command_name = self.commands.get(number-0x40, "UNKNOWN")
                    if (command_name != "UNKNOWN"):
                        ACK = True
                        long_text = "ACK "+command_name
                        mid_text = "ACK %d" % number
                        short_text = "%d" % number

                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])



                if command_name == "UNKNOWN":
                    long_text = command_name
                    mid_text = command_name
                    short_text = command_name
                else:
                    long_text = "Valid Command"
                    mid_text = "Valid"
                    short_text = "Valid"

                self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])


            if i==5:
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                long_text = "Checksum1 %d" % number
                mid_text = "CS1 %d" % number
                short_text = "%d" % number

                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

                if (self.validCheckSum1() == number):
                    long_text = "Valid Checksum"
                    mid_text = "Valid Check"
                    short_text = "Valid"
                else:
                    long_text = "Invalid Checksum"
                    mid_text = "Invalid Check"
                    short_text = "Invalid"

                self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])

            if i == 6:
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                long_text = "Checksum2 %d" % number
                mid_text = "CS2 %d" % number
                short_text = "%d" % number

                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])


                if (self.validCheckSum2() == number):
                    long_text = "Valid Checksum"
                    mid_text = "Valid Check"
                    short_text = "Valid"
                else:
                    long_text = "Invalid Checksum"
                    mid_text = "Invalid Check"
                    short_text = "Invalid"

                self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])



        if command_name=="UNKNOWN":
            for i in range(7,self.RecordLength):
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                long_text = "Packet data %d  " % number
                mid_text = "Pd %d" % number
                short_text = "%d" % number

                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

        elif command_name=="RAM_WRITE":
            self.RAM_Write(ACK)

        elif command_name=="RAM_READ":
            self.RAM_Read(ACK)

        elif command_name=="EEP_WRITE":
            self.ROM_Write(ACK)

        elif command_name=="EEP_READ":
            self.ROM_Read(ACK)

        elif command_name=="I_JOG":
            self.I_JOG(ACK)

        elif command_name == "S_JOG":
            self.S_JOG(ACK)

        elif command_name=="STAT":
            self.STAT(ACK)

        elif command_name=="ROLLBACK":
            self.ROLLBACK(ACK)

        elif command_name=="REBOOT":
            self.REBOOT(ACK)






        self.reset()


    def validCheckSum1(self):
        return self.validCheckSumSub()&0xFE

    def validCheckSum2(self):
        return (~(self.validCheckSumSub())) & 0xFE


    def validCheckSumSub(self):
        PacketSize = self.RecordData[2].data
        PID = self.RecordData[3].data
        CMD = self.RecordData[4].data

        Sum = PacketSize^PID^CMD

        for i in range(7,self.RecordLength):
         Sum = Sum ^ self.RecordData[i].data

        return Sum



    def ROM_Write(self, ACK):
        return

    def ROM_Read(self, ACK):
        return

    def RAM_Write(self, ACK):
        RamIndex = 7
        RamObject = self.GetMemFromRAM(self.RecordData[RamIndex].data)



        #Name tag
        name = RamObject.Type
        start = self.RecordData[RamIndex].start
        end = self.RecordData[RamIndex].end

        long_text = "RAM: " + name
        mid_text = name
        short_text = name

        self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

        # Permission tag
        Permission = RamObject.Perm
        long_text = "Permission: " + Permission
        mid_text = "Perm: " + Permission
        short_text = Permission

        self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])


        RamIndex = RamIndex + 1





        # Sub length
        start = self.RecordData[RamIndex].start
        end = self.RecordData[RamIndex].end
        number = self.RecordData[RamIndex].data

        long_text = "Sub Length: %d" % number
        mid_text = "SL %d" % number
        short_text = "%d" % number
        self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

        if (number != RamObject.Bytes):
            start = self.RecordData[RamIndex].start
            end = self.RecordData[RamIndex].end

            long_text = "Invalid expected %d" % RamObject.Bytes
            mid_text = "Invalid != %d" % RamObject.Bytes
            short_text = "Invalid"
            self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])

        RamIndex = RamIndex + 1

        #Packet Sub data
        if (RamObject.Fucntion != None):
            RamObject.Fucntion(RamIndex,self.RecordData)

        else:
            # Offet for the ACK
            Offet = 0
            if (ACK):
                Offet = 2
            for i in range(9, self.RecordLength-Offet): #Data values
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                long_text = "Value: %d" % number
                mid_text = "Val %d" % number
                short_text = "%d" % number

                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])



        if (ACK):
            self.StatusError(self.RecordLength - 2, self.RecordData)
            self.StatusDetail(self.RecordLength - 1, self.RecordData)


        return

    def RAM_Read(self, ACK):
        RamIndex = 7
        RamObject = self.GetMemFromRAM(self.RecordData[RamIndex].data)

        # Name tag
        name = RamObject.Type
        start = self.RecordData[RamIndex].start
        end = self.RecordData[RamIndex].end

        long_text = "RAM: " + name
        mid_text = name
        short_text = name

        self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

        # Permission tag
        Permission = RamObject.Perm
        long_text = "Permission: " + Permission
        mid_text = "Perm: " + Permission
        short_text = Permission

        self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])

        RamIndex = RamIndex + 1

        # Sub length
        start = self.RecordData[RamIndex].start
        end = self.RecordData[RamIndex].end
        number = self.RecordData[RamIndex].data

        long_text = "Sub Length: %d" % number
        mid_text = "SL %d" % number
        short_text = "%d" % number
        self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

        RamIndex = RamIndex + 1

        # Packet Sub data
        if (RamObject.Fucntion != None):
            RamObject.Fucntion(RamIndex, self.RecordData)

        else:
            # Offet for the ACK
            Offet = 0
            if(ACK):
                Offet = 2

            for i in range(9, self.RecordLength - Offet):  # Data values
                start = self.RecordData[i].start
                end = self.RecordData[i].end
                number = self.RecordData[i].data

                long_text = "Value: %d" % number
                mid_text = "Val %d" % number
                short_text = "%d" % number

                self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])


        if(ACK):
            self.StatusError(self.RecordLength-2,self.RecordData)
            self.StatusDetail(self.RecordLength-1,self.RecordData)


    def I_JOG(self, ACK):
        return

    def S_JOG(self, ACK):

        if (not ACK):
            Index = 7

            # Playtime
            start = self.RecordData[Index].start
            end = self.RecordData[Index].end
            number = self.RecordData[Index].data

            time = number * 11.2

            long_text = "PTime: %.1fms" % time
            mid_text = "PT: %.1fms" % time
            short_text = "%.1f" % time

            self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

            Index = Index + 1


            # Position
            JOG_LSB = self.RecordData[Index].data
            JOG_MSB = self.RecordData[Index + 1].data
            Position = (JOG_MSB << 8) | JOG_LSB
            start = self.RecordData[Index].start
            end = self.RecordData[Index + 1].end
            long_text = "Goal %d" % Position
            mid_text = "%d" % Position
            short_text = "%d" % Position
            self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

            Index = Index + 2

            # Set
            start = self.RecordData[Index].start
            end = self.RecordData[Index].end
            SET = self.RecordData[Index].data

            #Flips the bit order
            SET = int('{:08b}'.format(SET)[::-1], 2)



            # Set -> Direction
            if (SET & 0b00000001) == 0:
                direction = "CCW"
            else:
                direction = "CW"

            long_text = direction
            mid_text = direction
            short_text = direction

            SubStart = self.SubTime(start, end, 8 , 10)
            SubEnd = self.SubTime(start, end, 9, 10)
            self.put(SubStart, SubEnd, self.out_ann, [0, [long_text, mid_text, short_text]])




            # Set -> Mode
            if (SET & 0b10000000) == 0:
                long_text = "Position"
                mid_text = "Posi"
                short_text = "P"
                mode = "Position"
                mode
            else:
                long_text = "Continuous"
                mid_text = "Cont"
                short_text = "C"
                mode = "Continuous"

            SubStart = self.SubTime(start, end, 1, 10)
            SubEnd = self.SubTime(start, end, 2, 10)
            self.put(SubStart, SubEnd, self.out_ann, [0, [long_text, mid_text, short_text]])


            # Set -> LEDs
            red_led = (SET & 0b00010000) != 0
            green_led = (SET & 0b00100000) != 0
            blue_led = (SET & 0b01000000) != 0

            # Combine LED info into a readable color name (optional)
            if red_led and green_led and blue_led:
                led_color = "White"
            elif red_led and green_led:
                led_color = "Soft Green"
            elif red_led and blue_led:
                led_color = "Pink"
            elif green_led and blue_led:
                led_color = "Cyan"
            elif red_led:
                led_color = "Red"
            elif green_led:
                led_color = "Green"
            elif blue_led:
                led_color = "Blue"
            else:
                led_color = "Off"

            long_text = led_color
            mid_text = led_color
            short_text = led_color
            SubStart = self.SubTime(start, end, 2, 10)
            SubEnd = self.SubTime(start, end, 5, 10)
            self.put(SubStart, SubEnd, self.out_ann, [0, [long_text, mid_text, short_text]])

            long_text = "Reserved"
            mid_text = "Reserved"
            short_text = "---"
            SubStart = self.SubTime(start, end, 5, 10)
            SubEnd = self.SubTime(start, end, 8, 10)
            self.put(SubStart, SubEnd, self.out_ann, [0, [long_text, mid_text, short_text]])

            Index = Index + 1


            # Servo ID
            start = self.RecordData[Index].start
            end = self.RecordData[Index].end
            ServoId = self.RecordData[Index].data
            long_text = "Servo ID %d" % ServoId
            mid_text = "ID %d" % ServoId
            short_text = "%d" % ServoId



            self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])
        else:
            self.StatusError(self.RecordLength - 2, self.RecordData)
            self.StatusDetail(self.RecordLength - 1, self.RecordData)
        return

    def STAT(self, ACK):
        return

    def ROLLBACK(self, ACK):
        return

    def REBOOT(self, ACK):
        return




# Function pointers

    def TorqueControl(self, Index, RecordData):
        Target = RecordData[Index]
        start = Target.start
        end = Target.end

        long_text = "UNKNOWN"
        mid_text = "UNKNOWN"
        short_text = "UNKNOWN"

        if (Target.data == 0x40):
            long_text = "Break On"
            mid_text = "Break"
            short_text = "Break"

        if (Target.data == 0x60):
            long_text = "Torque On"
            mid_text = "On"
            short_text = "On"

        if (Target.data == 0x00):
            long_text = "Torque Free"
            mid_text = "Free"
            short_text = "Free"

        self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])

        if (long_text == "UNKNOWN"):
            long_text = "Invalid"
            mid_text = "Invalid"
            short_text = "Invalid"
            self.put(start, end, self.out_ann, [1, [long_text, mid_text, short_text]])

    def StatusError(self, Index, RecordData):
        Target = RecordData[Index]
        start = Target.start
        end = Target.end

        if (Target.data == 0x00):
            long_text = "Error Clean"
            mid_text = "Clean"
            short_text = "Clean"
            self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])
        else:
            ErrorNames = ["Exceed input Voltage limit",
                          "Exceed allowed Voltage limit",
                          "Exceed Temperature Limit",
                          "Invalid Packet",
                          "Overload detected",
                          "Reserved",
                          "EEP REG distorted",
                          "Reserved",
                          ]

            Byte = int('{:08b}'.format(Target.data)[::-1], 2)

            for i in range(8):
                SubStart = self.SubTime(start, end, i + 1, 10)
                SubEnd = self.SubTime(start, end, i + 2, 10)

                ErrorState = (Target.data  >> i) & 1

                if (ErrorState == 1):
                    long_text = ErrorNames[i]
                    mid_text = ErrorNames[i]
                    short_text = ErrorNames[i]
                else:
                    long_text = mid_text = short_text = ""
                self.put(SubStart, SubEnd, self.out_ann, [0, [long_text, mid_text, short_text]])

    def StatusDetail(self, Index, RecordData):
        Target = RecordData[Index]
        start = Target.start
        end = Target.end
        ErrorNames = ["Moving Flag",
                      "Inposition Flag",
                      "Checksum Error",
                      "Unknown Command",
                      "Exceed REG range",
                      "Garbage detected",
                      "Torque ON",
                      "Reserved",
                      ]

        Byte = int('{:08b}'.format(Target.data)[::-1], 2)

        for i in range(8):
            SubStart = self.SubTime(start, end, i + 1, 10)
            SubEnd = self.SubTime(start, end, i + 2, 10)

            ErrorState = (Target.data >> i) & 1

            if(ErrorState == 1):
                long_text = ErrorNames[i]
                mid_text = ErrorNames[i]
                short_text = ErrorNames[i]
            else:
                long_text = mid_text = short_text = ""

            self.put(SubStart, SubEnd, self.out_ann, [0, [long_text, mid_text, short_text]])




    def ACK(self, Index, RecordData):
        Target = RecordData[Index]
        start = Target.start
        end = Target.end

        long_text = "Invalid"
        mid_text = "Invalid"
        short_text = "Invalid"

        if (Target.data == 0):
            long_text = "No reply"
            mid_text = "No reply"
            short_text = "No reply"

        if (Target.data == 1):
            long_text = "CMD"
            mid_text = "Read CMD"
            short_text = "Only reply to Read CMD"

        if (Target.data == 2):
            long_text = "All"
            mid_text = "Reply to all "
            short_text = "Reply to all Request Packets"

        self.put(start, end, self.out_ann, [0, [long_text, mid_text, short_text]])



    def GetMemFromRAM(self, Ram_Number):
        for i in range(len(self.MotorMemory)):
            if self.MotorMemory[i].RAM == Ram_Number:
                return self.MotorMemory[i]
        return None

    def GetMemFromROM(self, Rom_Number):
        for i in range(len(self.MotorMemory)):
            if self.MotorMemory[i].ROM == Rom_Number:
                return self.MotorMemory[i]
        return None

    def GetMemFromName(self, Name):
        for i in range(len(self.MotorMemory)):
            if self.MotorMemory[i].Type == Name:
                return self.MotorMemory[i]
        return None



    def SubTime(self, start, end, target, InBetweens):
        distance = float(end - start)
        value = (distance / float(InBetweens)) * float(target)
        return start + int(round(value))


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

            # STARTBIT
            # if ptype == 'STARTBIT':
            #     self.Index = 0
            #     self.PacketData = []
            #     return

            # DATA: ['DATA', rxtx, (value, databits_list)]
            if ptype == 'DATA':
                try:
                    payload = data[2]
                    val = payload[0] if isinstance(payload, (list, tuple)) else int(payload)
                except Exception:
                    # Post to error row
                    self.put(ss, es, self.out_ann, [1, ["Invalid DATA payload", "Bad payload"]])
                    return

                #self.decode_byte_Collect(val, ss, es)


                return

            # FRAME: ['FRAME', rxtx, (value, valid)]
            if ptype == 'FRAME':
                try:
                    val, valid = data[2]
                except Exception:
                    return

                #printing = ["Error", "E", "E"]
                # if (self.DataIndex < 7):
                #printing = self.decode_byte_Check(ss,es)
                self.Record(val, ss, es)
                # else:
                #     long_text = "Data: 0x%02X" % val
                #     mid_text = "D: 0x%02X" % val
                #     short_text = "0x%02X" % val
                #     printing = [long_text, mid_text, short_text]

                #self.put(ss, es, self.out_ann, [0, printing])
                # self.put(ss, es, self.out_ann, [1, "%d" % (self.Index-1)])


                # if valid:
                #     long_text = "Frame: 0x%02X" % val
                #     short_text = "%02X" % val
                #     self.put(ss, es, self.out_ann, [0, [long_text, short_text]])
                # else:
                #     long_text = "Frame error: 0x%02X" % val
                #     short_text = "Frame err"
                #     self.put(ss, es, self.out_ann, [1, [long_text, short_text]])
                return

            # PARITY ERROR
            if ptype == 'PARITY ERROR':
                try:
                    expected, actual = data[2]
                except Exception:
                    return
                long_text = "Parity error exp=%d act=%d" % (expected, actual)
                short_text = "Parity err"
                self.put(ss, es, self.out_ann, [1, [long_text, short_text]])
                return

            # INVALID STARTBIT / STOPBIT
            if ptype == 'INVALID STARTBIT':
                try:
                    val = data[2]
                except Exception:
                    val = None
                long_text = "Invalid start bit: %s" % str(val)
                short_text = "Start err"
                self.put(ss, es, self.out_ann, [1, [long_text, short_text]])
                return

            if ptype == 'INVALID STOPBIT':
                try:
                    val = data[2]
                except Exception:
                    val = None
                long_text = "Invalid stop bit: %s" % str(val)
                short_text = "Stop err"
                self.put(ss, es, self.out_ann, [1, [long_text, short_text]])
                return

            # BREAK
            if ptype == 'BREAK':
                self.put(ss, es, self.out_ann, [0, ["Break condition", "Break"]])
                return

            # IDLE
            if ptype == 'IDLE':
                #self.put(ss, es, self.out_ann, [0, ["Idle", "Idle"]])
                return

            # fallback (debug)
            # self.put(ss, es, self.out_ann, [1, ["Unknown pkt:%s" % ptype, str(data)]])
