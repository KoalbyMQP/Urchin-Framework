import time

from Crab import Crab, TicketType
from HerkuleX import HerkuleX, JogLedColor, HerkulexModel, HerkuleXCommandNames
from Reaction import Reaction
from Bridge import Bridge




def main() -> None:
    test = Crab("bridge.toml",True)

    time.sleep(3)

    command = HerkuleX.MoveOne("Hip", float(0), 40, JogLedColor.LED_BLUE)
    #command = HerkuleX.MoveOne("Hip", float(-23), 40, JogLedColor.LED_BLUE)
    #command = HerkuleX.MoveOne("Hip", float(23), 40, JogLedColor.LED_RED)
    ticket = test.send(TicketType.Asynchronous,[command],None,False)
    print("Main:ticket" + str(ticket))
    test.close()



if __name__ == '__main__':
    # cProfile.run("main()")
    main()
