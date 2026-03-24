import struct
import tomlkit_extras
from tomlkit_extras import load_toml_file, TOMLDocumentDescriptor, InvalidFieldError, InvalidArrayOfTablesError
from typing import Union
from ESPSerial import *
import os

def IntSToByte(number:int)->bytes:
    return bytes(str(number), 'utf-8')

class Motor:
    def __init__(self, Number:int, Brand:str, Model:str, Joint:str, Bounds:list[int], AlignmentAngle:int):
        self.Number = Number
        self.Brand = Brand.encode('utf-8')
        self.Model = Model.encode('utf-8')
        self.Joint = Joint.encode('utf-8')
        self.Bounds = Bounds
        self.AlignmentAngle = AlignmentAngle

    def print(self):
        print(self.Number)
        print(self.Brand)
        print(self.Model)
        print(self.Joint)
        print(str(self.Bounds[0])+":"+str(self.Bounds[1]))
        print(self.AlignmentAngle)
        print()

class Bridge:
    VPID: int = 0
    Path: str = ""
    Version: str = "0.1"
    data: dict = None

    Motors:list[Motor] = []

    MotorName = "Motor"
    Motor_format = [["MotorNum",int],
                    ["MotorBrand",str],
                    ["MotorModel",str],
                    ["JointName",str],
                    ["Bounds", list, int],
                    ["AlignmentAngle", int]]

    MaxNum:int = 253
    MaxBrand:int = 30
    MaxModel:int = 10
    MaxName:int = 50

    ##Struct
    MotorNumFormat:bytes = b'B'
    MotorBrandFormat:bytes = IntSToByte(MaxBrand) + b's'
    MotorModelFormat:bytes = IntSToByte(MaxModel) + b's'
    JointNameFormat:bytes = IntSToByte(MaxName) + b's'
    BoundsMinFormat:bytes = b'i'
    BoundsMaxFormat: bytes = b'i'
    AlignmentAngleFormat: bytes = b'i'
    NextNodeFormat: bytes = b'?'


    DataStruct:bytes = \
        b'<' + \
        MotorNumFormat + \
        MotorBrandFormat + \
        MotorModelFormat + \
        JointNameFormat + \
        BoundsMinFormat + \
        BoundsMaxFormat + \
        AlignmentAngleFormat + \
        NextNodeFormat

    def __init__(self,Path: str, serial:ESPSerial):
        # --- Set up the path ---
        self.Path = Path
        self.absolute_path = os.path.abspath(self.Path)
        self.Serial = serial


        print(self.DataStruct)

        # --- Read and parse TOML file ---
        with open(self.Path, "r", encoding="utf-8") as f:
            raw = f.read()

        toml_doc = load_toml_file(raw)
        desc = TOMLDocumentDescriptor(toml_doc)

        # --- Get and check file version ---
        try:
            field = desc.get_field(hierarchy="meta.Version")

        except InvalidFieldError:
            # If meta.Version not found
            raise TomlValueError(f"\"Version\" is missing in .toml")

        line = field.line_no
        value = field.value
        FileVersion = value

        if not isinstance(FileVersion, str):
            raise TomlValueError(f"Version in .toml must be a string, got {type(FileVersion).__name__}", self.absolute_path, line)

        if FileVersion != self.Version:
            raise TomlValueError(f"File version mismatch: expected {self.Version}, found {FileVersion}", self.absolute_path, line)

        # --- Get the list of Motors from the .toml ---
        try:
            Motors = desc.get_aot(hierarchy=self.MotorName)

        except InvalidArrayOfTablesError:
            raise TomlValueError(f"\"{self.MotorName}\"s are missing in .toml",self.absolute_path)

        # --- Check all "Motor"s in .toml file ---
        Motors = Motors[0].tables[self.MotorName]

        for index, motor in enumerate(Motors, start=1):
            outPut=self.validateMotor(motor,index)

            # --- Print error if one was found ---
            if outPut is not None:
                raise TomlValueError(outPut.get("Message"),self.absolute_path,outPut.get("Line"))
            #else:
            #    self.Motors.append()

        # Get the data into the objects


        # Add to the ROS Posting



        # Send the values to the ESP-32


        for index, motor in enumerate(Motors, start=1):
            self.Motors.append(self.AddMotor(motor,index))

        for index, motor in enumerate(self.Motors, start=1):
            motor.print()



    def validateMotor(self, motor:list[tomlkit_extras.descriptor._descriptors.TableDescriptor], index:int) -> Union[dir, None]:
        MotorHeaderLine = motor.line_no
        size=len(self.Motor_format)
        for task in self.Motor_format:
            attribute = motor.fields.get(task[0],None)
            if attribute is None:
                # Failed
                return self.ValadatePack(f"{task[0]} is missing from Motor",MotorHeaderLine)
            else:
                # Passed
                line:int = attribute.line_no
                var:any = attribute.value
                Type:type = attribute.value_type
                targetType = ""
            if task[1] is list:
                targetType = task[2]

                # Check if it's a list
                if not isinstance(attribute.value,list):
                    return self.ValadatePack(f"\"{task[0]}\" is not a list", line)

                # Check if list size is 2 and our only list is bounds
                elif len(attribute.value) != 2:
                    return self.ValadatePack(f"\"{task[0]}\" list is not size of 2", line)

                # check the variable type
                elif not self.CheckList(attribute.value,targetType):
                    return self.ValadatePack(f"Values in \"{task[0]}\" list is not an {str(targetType)}", line)
            else:
                targetType = task[1]

                # check variable type
                if Type != targetType:
                    return self.ValadatePack(f"Value in \"{task[0]}\" is not an {str(targetType)}", line)
        return None


    def GetMotor(self, motor:list[tomlkit_extras.descriptor._descriptors.TableDescriptor]) -> Motor:
        print(f"\nGetMotor")






    def CheckList(self, List:list, Type:type) -> bool:
        for item in List:
            if type(item) is not Type:
                return False
        return True

    @staticmethod
    def ValadatePack(Message:str, Line:int) -> dir:
        return {"Message":Message, "Line":Line}



    def AddMotor(self, motor:list[tomlkit_extras.descriptor._descriptors.TableDescriptor], index:int) -> Motor:
        Number = 0
        Brand = ""
        Model = ""
        Joint = ""
        Bounds = []
        AlignmentAngle = 0

        Number = motor.fields.get(self.Motor_format[0][0],None).value
        Brand = motor.fields.get(self.Motor_format[1][0], None).value
        Model = motor.fields.get(self.Motor_format[2][0], None).value
        Joint = motor.fields.get(self.Motor_format[3][0], None).value
        Bounds = motor.fields.get(self.Motor_format[4][0], None).value
        AlignmentAngle = motor.fields.get(self.Motor_format[5][0], None).value

        return Motor(Number,Brand,Model,Joint,Bounds,AlignmentAngle)

    def SendAll(self) -> None:
        for index, motor in enumerate(self.Motors, start=1):
            if index == len(self.Motors):
                self.Send(motor,True)
            else:
                self.Send(motor,False)

    def Send(self,motor:Motor, last:bool) -> None:
        packed_data = struct.pack(self.DataStruct, motor.Number, motor.Brand, motor.Model, motor.Joint, motor.Bounds[0], motor.Bounds[1], motor.AlignmentAngle, last)
        self.Serial.send_packet(0,b"BridgeAdd" + packed_data)

# class FileVersionError(Exception):
#     """Raised when a file has an unsupported or incorrect version."""
#     def __init__(self, expected_version, found_version, line, message=None):
#         if message is None:
#             message = f"File version mismatch: expected {expected_version}, found {found_version} one line {line}"
#         super().__init__(message)
#         self.expected_version = expected_version
#         self.found_version = found_version

class TomlValueError(Exception):
    def __init__(self, message, file_path=None, line_number=None):
        super().__init__(message)
        self.file_path = file_path
        self.line_number = line_number

    def print(self):
        header = f"TOML Value Error: {self.args[0]}"
        if self.file_path and self.line_number:
            header += f"\n{self.file_path}:{self.line_number}"  # JetBrains clickable
        print(header)
