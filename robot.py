from typing import Callable
from cyberonics_py import Robot, Device, Target
from cyberonics_py.graphics import Button, GraphicCell
import os
import signal
import subprocess


# from .targets import Depthai
class Finley(Robot):
    def __init__(self):
        self.control_cell = ControlCell()
        super().__init__([self.control_cell], [Urchin_Test(self)])



class ControlCell(Device):
    def __init__(self):

        self.listeners: [Callable] = []

        def button_pressed():
            print("Button pressed!")
            for listener in self.listeners:
                listener()

        button = Button(text="Press me", onclick=button_pressed)
        super().__init__(properties=[], graphic_cell=GraphicCell([button]))

    def listen(self, listener: Callable):
        self.listeners.append(listener)



class Urchin_Test(Target):
    def __init__(self, robot: Robot):
        super().__init__("Urchin Test", robot)
        self.process = None

    def _run(self):
        #Run script
        print("Running shell script with Popen")

        subprocess.run(["python", "Crab_API/src/main.py"])

        return 1

    async def _shutdown(self, beat):
        if self.process:
            ##Terminate the process group
            pgid = os.getpgid(self.process.pid)
            os.killpg(pgid, signal.SIGTERM)
            ##If process ignors sigterm, force the shutdown by killing the process
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(pgid, signal.SIGNALKILL)