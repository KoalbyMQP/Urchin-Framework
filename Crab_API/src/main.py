import time

from Crab import Crab, TicketType
from HerkuleX import HerkuleX, JogLedColor, HerkulexModel, HerkuleXCommandNames
from Reaction import Reaction
from Bridge import Bridge




def main() -> None:




    test = Crab("bridge.toml",True)

    time.sleep(3)


    while True:
        test.GetHealth("AckCheck")





        #command = HerkuleX.SetTorque(True,"",True)
        #ticket = test.send(TicketType.Asynchronous, [command], None, False)
        #print("Main:ticket" + str(ticket))
        #time.sleep(3)
        #def ReactFunction(code: int, value: int) -> None:
        #    print("Got code "+str(code)+" back "+str(value))

        #TestReact = Reaction("Shoulder",HerkuleXCommandNames.TestReact, 0,ReactFunction)

        #print("testing react")
        #time.sleep(1)
        #command = HerkuleX.TestReact("Shoulder", 69)
        #ticket = test.send(TicketType.Asynchronous, [command], [TestReact], False)
        #print("Main:ticket" + str(ticket))

        #print("end testing react")

        time.sleep(3)
        command = HerkuleX.MoveOne("Shoulder", float(-23), 40, JogLedColor.LED_BLUE)
        ticket = test.send(TicketType.Asynchronous,[command],None,False)
        print("Main:ticket" + str(ticket))

        #time.sleep(1)
        #print("\n\n\n\n\n\n")
        #command = HerkuleX.MoveOne("Hip", float(-23), 40, JogLedColor.LED_BLUE)
        #ticket = test.send(TicketType.Asynchronous, [command], None, False)
        #print("Main:ticket" + str(ticket))

        time.sleep(1)
        print("\n\n\n\n\n\n")
        command = HerkuleX.MoveOne("Shoulder", float(23), 40, JogLedColor.LED_RED)
        ticket = test.send(TicketType.Asynchronous, [command], None, False)
        print("Main:ticket" + str(ticket))

        #time.sleep(1)
        #print("\n\n\n\n\n\n")
        #command = HerkuleX.MoveOne("Hip", float(23), 40, JogLedColor.LED_RED)
        #ticket = test.send(TicketType.Asynchronous, [command], None, False)
        #print("Main:ticket" + str(ticket))


        time.sleep(1)
        test.GetHealth("AckCheck")

        time.sleep(1)


    test.close()



if __name__ == '__main__':
    # cProfile.run("main()")
    main()
