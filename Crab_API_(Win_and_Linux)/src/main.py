from Crab import *
from Types import *
import time
from HerkuleX import *
import sys
import signal
import cProfile






def main():

   test = Crab(True)


   command = HerkuleX.MoveOne(7, 500, 100, JogLedColor.LED_BLUE, HerkulexModel.MODEL_0601)
   ticket = test.send(TicketType.Asynchronous,[command],None,False)
   print("Main:ticket"+str(ticket))




  

   print("\n\n\n\n")



   command = HerkuleX.MoveOne(7, 1030, 200, JogLedColor.LED_GREEN, HerkulexModel.MODEL_0601)
   ticket = test.send(TicketType.Asynchronous, [command], None, False)
   print("Main:ticket" + str(ticket))



   test.close()


if __name__ == '__main__':
    #cProfile.run("main()")
    main()
