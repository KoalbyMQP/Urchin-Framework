from Crab import *
from Types import *
import time
from HerkuleX import *
import sys
import signal





if __name__ == '__main__':

   test = Crab(True)


   command = HerkuleX.MoveOne(7, 30, 3, JogLedColor.LED_RED, HerkulexModel.MODEL_0601)
   ticket = test.send(TicketType.Asynchronous,[command],None,False)
   print("Main:ticket"+str(ticket))




   time.sleep(2)

   print("\n\n\n\n")



   command = HerkuleX.MoveOne(7, 512, 27, JogLedColor.LED_RED, HerkulexModel.MODEL_0601)
   ticket = test.send(TicketType.Asynchronous, [command], None, False)
   print("Main:ticket" + str(ticket))



   test.close()




