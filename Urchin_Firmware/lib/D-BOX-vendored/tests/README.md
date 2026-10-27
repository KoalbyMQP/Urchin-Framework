# D-BOX receive() test (runs on a PC, no hardware)

Host-side test for this ESP32 copy of D-BOX. It compiles the real
`src/DBOXprotocol.cpp`, `src/IPort.cpp` and the ring buffer together with
`SimUart`, a fake `IPort` that feeds scripted bytes on a simulated clock
(`wait(ms)` advances it; every UART read costs 1 us). Each case runs in its
own process with a 5 s alarm, so a stuck receiver is reported as `FROZE`
instead of hanging the run.

| Case | What it checks |
|------|----------------|
| 1 | A normal packet |
| 2 | Bug 1: sender pauses 50 ms mid-header; receiver must rest, not spin |
| 3 | Bug 2: a false 0x07 right before a real packet |
| 4 | Bug 2: a corrupted header, then a good packet |
| 5 | Bug 2: a false 0x07 whose header overlaps the real 0x07 |
| 6 | Boot noise, a 0x07 inside a payload, two packets back to back |
| 7 | All of the above, arriving over time |

Output columns: `checks` = UART reads, `waits` = rests, `most checks w/o rest`
= longest run of reads with no rest (huge means spinning).

Result on 2026-10-26: fixed code passes 7/7; the previous code (commit e87e5e5)
froze on cases 3, 4, 5, 7 and made 50,262 checks without resting on case 2.

## Run it (Linux, WSL, or the Raspberry Pi)

From the `Urchin-Framework` folder:

    L=Urchin_Firmware/lib/D-BOX-vendored
    g++ -std=c++20 -include limits -include array -include bit -I $L/include \
        $L/tests/test_receive.cpp $L/src/DBOXprotocol.cpp $L/src/IPort.cpp \
        -o /tmp/test_receive && /tmp/test_receive

From Windows PowerShell (needs WSL with g++):

    wsl bash -lc "cd /mnt/c/<path-to>/Urchin-Framework && g++ -std=c++20 -include limits -include array -include bit -I Urchin_Firmware/lib/D-BOX-vendored/include Urchin_Firmware/lib/D-BOX-vendored/tests/test_receive.cpp Urchin_Firmware/lib/D-BOX-vendored/src/DBOXprotocol.cpp Urchin_Firmware/lib/D-BOX-vendored/src/IPort.cpp -o /tmp/test_receive && /tmp/test_receive"

Uses fork(), so it does not build with plain Windows MinGW. The `-include`
flags add standard headers DBOXprotocol.cpp uses without including (the ESP32
toolchain pulls them in). PlatformIO only compiles this library's `src/`, so
this folder never ends up in a firmware build.
