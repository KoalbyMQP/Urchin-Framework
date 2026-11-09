# urchin-fullstack: ESP32 sim -> ZaraOS VM -> Driver Station, all on this laptop.
# Start Crab from the Driver Station's App Store (Local: crab-api2 / arm64).
# Ctrl-C stops everything. Logs go to $ROOT/.sim-logs/.

# Settings (override with environment variables)
zaraos_vm="${ZARAOS_DEVENV:-$ROOT/../kernel-build/output/run-devenv.sh}"
zaraos_dir="${ZARAOS_DIR:-$ROOT/../ZaraOS}"
ds_dir="${DRIVERSTATION_DIR:-$ROOT/../ZaraOS-DriverStation}"
log_dir="$ROOT/.sim-logs"
mkdir -p "$log_dir"

# --- 1. Checks -------------------------------------------------------------
if [ ! -f "$zaraos_vm" ]; then
  echo "error: ZaraOS VM not found at $zaraos_vm" >&2
  echo "Build it: cd ZaraOS && nix develop && ./ZaraOS/scripts/build-qemu-devenv.sh (or set ZARAOS_DEVENV)" >&2
  exit 1
fi
if [ ! -d "$zaraos_dir" ]; then
  echo "error: ZaraOS repo not found at $zaraos_dir (set ZARAOS_DIR)" >&2
  exit 1
fi
if [ ! -d "$ds_dir/node_modules" ]; then
  echo "error: Driver Station not installed at $ds_dir" >&2
  echo "Run once: cd $ds_dir && nix develop && pnpm install (or set DRIVERSTATION_DIR)" >&2
  exit 1
fi

# 5555/5556 ESP32 UARTs, 8080 Cortex, 2222 VM SSH, 3000 Driver Station
for port in 5555 5556 8080 2222 3000; do
  if nc -z 127.0.0.1 "$port" 2>/dev/null; then
    echo "error: port $port is already in use (something from a previous run? try: pgrep -a qemu; pgrep -af next)" >&2
    exit 1
  fi
done

# --- Cleanup ---------------------------------------------------------------
# Each piece runs in its own process group (setsid), so killing the group
# also stops everything it started (QEMU, motor sim, pnpm -> next, ...).
sim_pid=""
vm_pid=""
ds_pid=""
stop_group() {
  if [ -n "$1" ]; then
    kill -TERM -- "-$1" 2>/dev/null || true
  fi
}
cleanup() {
  echo
  echo "Stopping Driver Station, ZaraOS VM and ESP32 sim..."
  stop_group "$ds_pid"
  stop_group "$vm_pid"
  stop_group "$sim_pid"
}
trap cleanup EXIT

# wait_for NAME SECONDS COMMAND...: retry COMMAND once a second until it succeeds.
wait_for() {
  local name="$1" seconds="$2"
  shift 2
  echo "Waiting for $name (up to ${seconds}s)..."
  for _ in $(seq 1 "$seconds"); do
    if "$@" >/dev/null 2>&1; then
      return 0
    fi
    sleep 1
  done
  echo "error: $name did not come up; see $log_dir" >&2
  exit 1
}

# --- 2. ESP32 sim ------------------------------------------------------------
setsid urchin-sim --no-build </dev/null >"$log_dir/esp32-sim.log" 2>&1 &
sim_pid=$!
wait_for "ESP32 sim (port 5555)" 30 nc -z 127.0.0.1 5555

# --- 3. ZaraOS VM ------------------------------------------------------------
# Uses ZaraOS's own flake for qemu-system-aarch64, the version that team tests with.
setsid nix develop "$zaraos_dir" --command "$zaraos_vm" </dev/null >"$log_dir/zaraos.log" 2>&1 &
vm_pid=$!
wait_for "ZaraOS Cortex (port 8080)" 180 curl -sf http://localhost:8080/health

# --- 4. Crab image in the VM? -----------------------------------------------
if ! curl -sf http://localhost:8080/images | grep -q '"crab-api2"'; then
  echo "warning: crab-api2 image is not in the VM yet. Load it once with:" >&2
  echo "  docker buildx build --platform linux/arm64 -f Crab_API2/Dockerfile --build-arg URCHIN_PORT=socket://10.0.2.2:5555 -t crab-api2:arm64 --load ." >&2
  echo "  docker save crab-api2:arm64 | ssh -p 2222 root@localhost nerdctl load" >&2
fi

# --- 5. Driver Station -------------------------------------------------------
# Uses the Driver Station's own flake for Node + pnpm.
setsid nix develop "$ds_dir" --command bash -c "cd '$ds_dir' && pnpm dev" </dev/null >"$log_dir/driver-station.log" 2>&1 &
ds_pid=$!
wait_for "Driver Station (port 3000)" 120 nc -z 127.0.0.1 3000

# --- 6. Ready ----------------------------------------------------------------
cat <<EOF

Full stack is up:
  Driver Station  http://localhost:3000   (connect to: localhost)
  Cortex API      http://localhost:8080
  VM SSH          ssh -p 2222 root@localhost   (password: zaraos)
Start Crab from the App Store: Local -> crab-api2 -> arm64.

Logs (watch one with: tail -f <file>):
  $log_dir/esp32-sim.log
  $log_dir/zaraos.log
  $log_dir/driver-station.log

Press Ctrl-C to stop everything.
EOF

# Keep running until Ctrl-C, or until one of the pieces stops by itself.
wait -n || true
echo "One of the pieces stopped on its own; check the logs in $log_dir" >&2
