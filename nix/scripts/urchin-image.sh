for f in bootloader.bin partitions.bin firmware.bin; do
  if [ ! -f "$BUILD/$f" ]; then
    echo "error: $BUILD/$f not found; run urchin-build first" >&2
    exit 1
  fi
done
esptool --chip esp32 merge-bin \
  -o "$IMAGE" --pad-to-size 4MB \
  0x1000 "$BUILD/bootloader.bin" \
  0x8000 "$BUILD/partitions.bin" \
  0x10000 "$BUILD/firmware.bin"
echo "Wrote $IMAGE"
