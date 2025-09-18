from Smart import *
from Types import *
import time

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
   test = Smart()


   command = Item(7,"MoveOne",[27, 30, "LED_GREEN"])
   ticket = test.send("A",[command],None,False)
   print(ticket)

   time.sleep(1)
   test.Thread.join()