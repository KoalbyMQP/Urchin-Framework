from ESPSerial import ESPSerial

ESP = ESPSerial()

# --- Main Loop ---
try:
    while True:
        ESP.read_packet()
        ESP.send_packet(0,b"GetHealth")
        print("trying to send packet")

finally:
    print("\nClosing serial port.")
    ESP.close()
