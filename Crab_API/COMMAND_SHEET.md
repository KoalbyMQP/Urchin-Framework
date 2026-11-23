# D-BOX Motor Command Sheet

**Overview:** This is the reference for talking to the motor board: every
command you can send, what each reply means, etc.

What the app sends to the ESP32, and what comes back. Firmware:
`Urchin_Firmware/src/DBoxMotor` (env `DBoxMotor`, v3).

> **Provisional:** stream letters and VPID meanings may still change once
> the team's stream/ID list is final. Command names and replies should not.

## The terms

| Term | Meaning |
|---|---|
| **D-BOX** | The team's message protocol between the Pi/laptop and the ESP32 (over USB serial, 115200 baud). Every message is one D-BOX packet. Ensure bytes arrive between both devices |
| **Packet** | Format: `0x07` start byte, VPID, stream, length, header CRC, payload, message CRC-32. A packet with a bad CRC is thrown away. |
| **VPID** | Who a packet is for. `0` = the board / all motors, `1`–`253` = the motor with that ID. Replies come back on the VPID the command used. (Essentially unique addresses for each motor)|
| **Stream** | Letter saying what kind of packet it is (table below). |
| **MarshalType Item** | Extends from DBOX protocol that ensures the pi & esp32 understand the bytes in the same way binary command format (`Clam/MarshalType`): `Item(joint, command, values)`. |
| **joint** | Which motor: its ID as text (`"10"`) or `"all"`. |
| **ID** | A motor's Herkulex ID, stored inside the motor|
| **CRC** | A checksum. The header CRC protects the packet's first bytes; the CRC-32 protects the whole message. |
| **S_JOG** | (MarshalType - Simultaenous Jog) One Herkulex packet: many motors, **one shared** travel time. All start and arrive together. |
| **I_JOG** | (MarshalType - Individual Jog) One Herkulex packet: many motors, **each its own** travel time. All start together, each arrives on its own schedule. |

## Streams

| Stream | Direction | Carries |
|---|---|---|
| `S` | both | Handshake (Initially connecting): both sides (esp32 and pi) send `"Urchin"` when connecting to each other |
| `M` | app → ESP32 | A MarshalType Item (the command) |
| `N` | app → ESP32 | The same commands as plain text, e.g. `Move 30 1000` (bench tools use this) |
| `E` | ESP32 → app | The reply: **exactly one per command**, text starting `OK` or `ERR` |
| `D` | ESP32 → app | An event nobody asked for, e.g. `AUTOSTOP` |

Send Items on **VPID 0**; the `joint` picks the motor. The reply comes back
on that motor's VPID (0 for `"all"`).

## Commands

Angles are degrees from center (about ±160). Times are milliseconds,
0–2856 (the servo's limit).

| Item | Does | Reply |
|---|---|---|
| `Item("all", "Info", [])` | Firmware version and known motors | `OK DBoxMotor v3 servos=10:0201,27:0101` |
| `Item("all", "Find", [])` | Scans both buses for motors (~20 s; wait up to 35 s) | `OK FOUND 10:0201 27:0101` |
| `Item("10", "Move", [30.0, 1000])` | Motor 10 to 30° in 1000 ms | `OK MOVE 30.0` |
| `Item("all", "Move", [0.0, 1000])` | Every motor to 0° (one packet each, ~10 ms apart) | `OK MOVE 0.0` |
| `Item("10", "Queue", [30.0])` | Store motor 10's target; doesn't move yet | `OK QUEUED 10 1` (ID, how many queued) |
| `Item("10", "Queue", [30.0, 500])` | Same, with its own travel time | `OK QUEUED 10 1` |
| `Item("all", "Go", [1000])` | Start everything queued, **one packet**. No own times → S_JOG, all take 1000 ms. Some own times → I_JOG, the 1000 ms fills in for the rest. | `OK GO 2 SJOG` / `OK GO 2 IJOG` |
| `Item("all", "Go", [])` | Same, when every queued motor has its own time (I_JOG) | `OK GO 2 IJOG` |
| `Item("all", "Clear", [])` | Empty the queue without moving | `OK CLEARED` |
| `Item("10", "Spin", [300])` | Turn continuously (−1023…1023). **Resend every ~100 ms**, or it stops after 0.5 s | `OK SPIN 300` |
| `Item("all", "Stop", [])` | Stop spinning | `OK STOP` |
| `Item("all", "Status", [])` | Each motor's error flags | `OK STAT 10 0 0 27 0 0` (ID, error, detail; `-` = no reply) |
| `Item("10", "SetId", [12])` | Give motor 10 the new ID 12 (saved in the motor) | `OK SETID 10 12` |

Bench-only tools (text on stream `N`, used by `bench_test.py`; the app
doesn't need them): `Model <0101|0201|0601|0602>`, `BaudScan`,
`FixBaud <id> <baud> [bus]`, `Sniff [id] [bus]`.

**Synchronized move, start to finish:**
```
Item("10",  "Queue", [30.0])    ->  OK QUEUED 10 1
Item("27",  "Queue", [-45.0])   ->  OK QUEUED 27 2
Item("all", "Go",    [1000])    ->  OK GO 2 SJOG      both move now, together motor 10 to 30 degrees, 27 to -45 both in a span of 1 second
```

## Errors

| Reply | Means |
|---|---|
| `ERR RANGE` | Angle is past what that motor can reach (nothing moved) |
| `ERR NOSERVO 10` | No motor 10 answered; run `Find`, check power/wiring |
| `ERR NOREPLY` | The motor didn't answer that request |
| `ERR EMPTY` | `Go` with nothing queued |
| `ERR FULL` | Queue is full (one slot per known motor) |
| `ERR UNKNOWN <word>` | Command name not recognised (check spelling/case) |
| `ERR USAGE ...` | Wrong values; the reply shows the right form |
| `ERR ITEM BADFORMAT` | The Item bytes didn't unpack |
| `ERR ITEM TOOLONG` | The command and values don't fit in one command (160 characters) |
| `ERR ITEM JOINT <x>` | `joint` was neither a number nor `"all"` |
| `ERR TAKEN` / `ERR BADID` | `SetId`: that ID is in use / not 1–253 |

**Events (stream `D`):** `AUTOSTOP` = a spinning motor stopped because the
keep-alive stopped arriving.

## Safety

Motors stop by themselves if the app closes, crashes, or USB is pulled
(spin needs the keep-alive). Moves already started finish their travel.
