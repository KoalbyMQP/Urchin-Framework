uart0="${URCHIN_SIM_UART0_PORT:-5555}"

if nc -z 127.0.0.1 "$uart0" 2>/dev/null; then
  echo "error: port $uart0 is already in use (another sim running? try: pgrep -a qemu)" >&2
  exit 1
fi

urchin-sim --no-build </dev/null & sim_pid=$!
trap 'pkill -P "$sim_pid" 2>/dev/null; kill "$sim_pid" 2>/dev/null || true' EXIT
echo "Waiting for the ESP32 sim on port $uart0..."
for _ in $(seq 1 30); do
  if nc -z 127.0.0.1 "$uart0"; then
    break
  fi
  sleep 1
done

if ! nc -z 127.0.0.1 "$uart0" 2>/dev/null; then
  echo "error: ESP32 sim did not start on port $uart0" >&2
  exit 1
fi

cd "$ROOT/Crab_API2"
# The port opens when QEMU starts, but the firmware needs a moment to boot.
# Crab sends the bridge right away without waiting for an ack, so connecting
# too early loses the first motor. TODO: wait for a real "firmware ready" signal.
sleep 1
URCHIN_PORT="socket://127.0.0.1:$uart0" uv run python examples/demo.py "$@"
