# Shared prologue for the helper scripts: locate the repo and firmware build dir.
find_repo() {
  if [ -n "${URCHIN_ROOT:-}" ]; then echo "$URCHIN_ROOT"; return; fi
  dir="$PWD"
  while [ "$dir" != "/" ]; do
    if [ -f "$dir/Urchin_Firmware/platformio.ini" ]; then echo "$dir"; return; fi
    dir="$(dirname "$dir")"
  done
  echo "error: could not find the Urchin-Framework repo (run inside it or set URCHIN_ROOT)" >&2
  exit 1
}
ROOT="$(find_repo)"
FW="$ROOT/Urchin_Firmware"
ENV_NAME="${URCHIN_PIO_ENV:-Urchin-Firmware}"
BUILD="$FW/.pio/build/$ENV_NAME"
IMAGE="${URCHIN_FLASH_IMAGE:-$FW/urchin_flash.bin}"
export ROOT FW ENV_NAME BUILD IMAGE

