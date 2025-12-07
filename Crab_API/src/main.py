import time

from Crab import Crab, TicketType
from HerkuleX import HerkuleX, JogLedColor, HerkulexModel
from Bridge import Bridge




def main() -> None:




    test = Crab("bridge.toml",True)

    time.sleep(1)

    command = HerkuleX.MoveOne("Sholder", float(-45), 40, JogLedColor.LED_BLUE)
    ticket = test.send(TicketType.Asynchronous,[command],None,False)
    print("Main:ticket" + str(ticket))

    time.sleep(1)
    print("\n\n\n\n\n\n")
    command = HerkuleX.MoveOne("Sholder", float(45), 40, JogLedColor.LED_GREEN)
    ticket = test.send(TicketType.Asynchronous, [command], None, False)
    print("Main:ticket" + str(ticket))
    test.close()


if __name__ == '__main__':
    # cProfile.run("main()")
    main()
