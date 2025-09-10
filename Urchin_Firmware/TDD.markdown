# Urchin Framework

Urchin Framework is a **smart motor driver** system designed to simplify and improve motor control through a combination of firmware and software. Its primary goal is to **abstract, synchronize, order, and enhance the safety** of motor and I²C chains, making complex motor networks easier and safer to manage.

---

# Objective
The goal of the project is to provide a safe and expandable platform for the Raspberry Pi to interface with motors and sensors with a seamless abstract interface.

---

## Goal
- The API will have the least amount of user-facing function call changes.
- The possibility of expansion of scope shall be planned for and included in the design.
- The framework should be adaptable to multiple purposes with minimal changes.
- Multiple instances of firmware shall be versioned from one tree with minimal file changes.

---

## Success Criteria
- [Define specific success metrics here based on testing and deployment.]

---

# Background
- [Provide project background, motivation, and context here.]

---

# Scope
- [Define the scope of the project, boundaries, and limitations.]

---

# Design

The Urchin framework is divided into four sectors, each with its own aquatic-themed name:

1. **Urchin Firmware**
2. **Crab API**
3. **Mollusk Peer**
4. **Coral Multiplexor**

---

## Lucid Chart
- [Include Lucidchart link or embed diagram showing architecture.]

---

## Overview
- [High-level overview of system architecture.]

---

## Infrastructure
- [Describe hardware and software infrastructure required.]

---

## Detailed Design
- [Explain internal design, modules, and data flow.]

---

## Expansion / Adaptability
- [Describe how the system can be scaled or adapted for other purposes.]

---

## Dependencies

### C
- [FreeRTOS](https://www.freertos.org/)
- [Espidf](https://idf.espressif.com/)

### Python
- Threading
- Serial

---

# Standards

## Python
- [Define Python coding standards here.]

## C/C++
**When should I use C or C++?**
- Prefer C unless exclusive C++ features are needed. Keep code as simple as possible.

### Power of Ten
Rules intended to eliminate certain coding practices:

1. Avoid complex flow constructs (goto, recursion).
2. All loops must have fixed bounds.
3. **Try to avoid heap memory allocation after initialization.**
4. Restrict functions to a single printed page.
5. Use a minimum of two runtime assertions per function.
6. Restrict scope of data to the smallest possible.
7. Check all return values of non-void functions.
8. Use preprocessor only for headers and simple macros. **Project-rule macros are allowed in `standards.h`.**
9. Limit pointer use to a single dereference; no function pointers except `static const`.
10. Compile with all warnings active; address all before release.

**Custom Rules**
1. Forbidden: `printf`, `fprintf`, `vprintf`, `vfprintf`. Use `PrintToPi` instead.
2. Changes to `Shipping.c`/`Shipping.h` require review by two project managers.
3. C++ strings are banned.
4. Use `pvPortMalloc` / `vPortFree` instead of `malloc` / `free`.

**ALL pull requests must meet these standards!**

---

## Git Policy

**Branch Policy:**
```
Main                    # Stable, production-ready
├── develop              # Integration branch
├── feature/package-name/feature-description
├── bugfix/package-name/bug-description
└── release/v1.2.0
```

**Python Project Structure:**
```
├── README.md
└── packages
    ├── common
    │     └── README.md
    └── foo
        ├── README.md
        ├── examples
        ├── scripts
        │   ├── build.py
        │   ├── release.py
        │   └── test.py
        └── test
```

**C/C++ Project Structure:**
```
├── "Project".README
├── include
├── lib
└── src
    ├── main.cpp
    └── ...
```

---

# Crab API

The Crab API provides user-friendly abstractions:

- Sends data to firmware easily.
- Manages code execution and flow:
    - Chained functions must resolve before continuing.
    - Send can execute functions on firmware completion or error.

### Function Calls
```
InitUrchin()
Ticket = SEND(Data, Type, LIST[ITEM], Restraints, LIST[Reaction], Chained)
CHECK(Ticket)
DROP(Ticket)
```

### Reaction
- Functions can be executed upon receiving a completion or error code from the firmware.

---

# Urchin Firmware

The firmware is the brains of the operation, running **FreeRTOS** on top of the **Espidf** framework.

### Communication

1. **PI ↔ Urchin**
    - Communicate via packets: `{VPID, Stream, Data, Checksum}`
    - Use UART; ESP32 UART-0 via USB, Raspberry Pi scans available COM ports.
    - If disconnected, Bluetooth can be used with the same packet structure.

2. **Motors and Sensors**
    - Communication via I²C and UART.
        - Dynamixel: uses its native packet system.
        - HerkuleX: uses its native packet system.
        - I²C: general sensor/motor communication.

3. **Peer-to-Peer**
    - Main Urchin firmware communicates with other ESP32s over CAN.
    - Wireless peers use ESPNOW (~6ms RTT) under special circumstances.

---

# Getting Started

### 1. Clone the repository
```bash
git clone https://github.com/KoalbyMQP/Urchin-Framework.git
cd Urchin-Framework
```

### 2. Setup Firmware
- Navigate to the firmware directories (e.g., `Urchin_Firmware`) and follow build instructions.

### 3. Integrate with Software
- Use the Crab API to control motors and I²C devices programmatically.

### 4. Run Safety Checks
- Ensure all motors and I²C devices are properly configured.

---

## Documentation

Generate API documentation using **Doxygen**:
```bash
doxygen Doxyfile
```
Documentation will be available in the `docs/` directory.

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](https://github.com/KoalbyMQP/Urchin-Framework/blob/main/LICENSE) file.
