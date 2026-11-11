#!/bin/bash
# Builds the DBoxMotor firmware for Linux against the simulated board in
# sim.cpp. Run from anywhere: bash Urchin_Firmware/tests/DBoxMotorSim/build.sh
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
F=$HERE/../..
# -include limits: the vendored DBOXprotocol.cpp relies on the ESP32
# toolchain pulling <limits> in for it.
g++ -std=gnu++20 -O1 -g -w -include limits -I"$HERE/include" \
  -I"$F/lib/D-BOX-vendored/include" -I"$F/lib/D-BOX-vendored/src" -I"$F/src/Herkulex" \
  "$HERE/sim.cpp" "$F/src/DBoxMotor/main.cpp" "$F/src/BenchShared/ServoBench.cpp" \
  "$F/src/BenchShared/ComsStub.cpp" "$F/src/Herkulex/Herkulex.cpp" \
  "$F/src/DBoxEcho/Esp32UartPort.cpp" \
  "$F/lib/D-BOX-vendored/src/DBOXprotocol.cpp" "$F/lib/D-BOX-vendored/src/IPort.cpp" \
  -o "$HERE/dboxmotor_sim" -lutil -lpthread
echo "built $HERE/dboxmotor_sim"
