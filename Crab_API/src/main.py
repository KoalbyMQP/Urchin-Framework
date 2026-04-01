import time

from Crab import Crab, TicketType
from HerkuleX import HerkuleX, JogLedColor, HerkulexModel
from Bridge import Bridge




def main() -> None:




    test = Crab("bridge.toml",True)

    time.sleep(1)


    while True:

        test.GetHealth("AckCheck")





        #command = HerkuleX.SetTorque(True,"",True)
        #ticket = test.send(TicketType.Asynchronous, [command], None, False)
        #print("Main:ticket" + str(ticket))

        time.sleep(1)
        command = HerkuleX.MoveOne("Shoulder", float(-23), 40, JogLedColor.LED_BLUE)
        ticket = test.send(TicketType.Asynchronous,[command],None,False)
        print("Main:ticket" + str(ticket))

        time.sleep(1)
        print("\n\n\n\n\n\n")
        command = HerkuleX.MoveOne("Hip", float(-23), 40, JogLedColor.LED_BLUE)
        ticket = test.send(TicketType.Asynchronous, [command], None, False)
        print("Main:ticket" + str(ticket))

        time.sleep(1)
        print("\n\n\n\n\n\n")
        command = HerkuleX.MoveOne("Shoulder", float(23), 40, JogLedColor.LED_RED)
        ticket = test.send(TicketType.Asynchronous, [command], None, False)
        print("Main:ticket" + str(ticket))

        time.sleep(1)
        print("\n\n\n\n\n\n")
        command = HerkuleX.MoveOne("Hip", float(23), 40, JogLedColor.LED_RED)
        ticket = test.send(TicketType.Asynchronous, [command], None, False)
        print("Main:ticket" + str(ticket))


        time.sleep(1)
        test.GetHealth("AckCheck")

        time.sleep(1)


    test.close()



if __name__ == '__main__':
    # cProfile.run("main()")
    main()
