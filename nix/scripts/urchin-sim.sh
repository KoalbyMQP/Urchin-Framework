usage() {
  cat <<EOF
Usage: urchin-sim [--no-build] [--no-motors] [-- extra qemu args...]

Builds the Urchin firmware, merges it into a 4MB flash image and boots it
in Espressif's QEMU. UART0 and UART1 are exposed as TCP servers, and
urchin-motor-sim is attached to UART1 to emulate the HerkuleX servos.

  --no-build   Skip 'pio run' and reuse the existing build output
  --no-motors  Don't start urchin-motor-sim (leaves UART1 free for other tools)
  -h, --help   Show this help

Environment:
  URCHIN_SIM_UART0_PORT  TCP port for UART0 (default 5555)
  URCHIN_SIM_UART1_PORT  TCP port for UART1 (default 5556)
  URCHIN_SIM_BRIDGE      bridge.toml naming the emulated servos (default:
                         emulate every servo ID the firmware addresses)
  URCHIN_SIM_MOTOR_ARGS  Extra urchin-motor-sim arguments (e.g. "-v")
  URCHIN_PIO_ENV         PlatformIO env (default Urchin-Firmware)
  URCHIN_FLASH_IMAGE     Flash image path (default Urchin_Firmware/urchin_flash.bin)
  URCHIN_ROOT            Repo root (default: found by walking up from \$PWD)

Point the Crab API at it with:
  URCHIN_PORT=socket://127.0.0.1:\${URCHIN_SIM_UART0_PORT:-5555}
Quit QEMU with Ctrl-A X.
EOF
}

build=1
motors=1
while [ $# -gt 0 ]; do
  case "$1" in
    --no-build) build=0; shift ;;
    --no-motors) motors=0; shift ;;
    -h|--help) usage; exit 0 ;;
    --) shift; break ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

if [ "$build" = 1 ]; then
  urchin-build
fi
urchin-image

uart0="${URCHIN_SIM_UART0_PORT:-5555}"
uart1="${URCHIN_SIM_UART1_PORT:-5556}"

if [ "$motors" = 1 ]; then
  motor_args=(--port "$uart1" --exit-on-disconnect --connect-timeout 30)
  if [ -n "${URCHIN_SIM_BRIDGE:-}" ]; then
    motor_args+=(--bridge "$URCHIN_SIM_BRIDGE")
  fi
  # shellcheck disable=SC2206 # split URCHIN_SIM_MOTOR_ARGS into words
  motor_args+=(${URCHIN_SIM_MOTOR_ARGS:-})
  # Waits for QEMU's UART1 server to come up, and exits when QEMU closes it.
  urchin-motor-sim "${motor_args[@]}" &
  motor_pid=$!
  trap 'kill "$motor_pid" 2>/dev/null || true' EXIT
fi

echo "Starting ESP32 simulator: UART0 -> tcp://127.0.0.1:$uart0, UART1 -> tcp://127.0.0.1:$uart1 (Ctrl-A X to quit)"
qemu-system-xtensa -nographic -machine esp32 \
  -drive "file=$IMAGE,if=mtd,format=raw" \
  -serial "tcp::$uart0,server=on,wait=off" \
  -serial "tcp::$uart1,server=on,wait=off" \
  "$@"
