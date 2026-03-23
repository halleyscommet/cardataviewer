import serial

port = "/dev/cu.usbmodem1101"
baud = 115200

with serial.Serial(port, baud, timeout=1) as ser:
    with open("revved.txt", "w") as f:
        print("Logging... Ctrl+C to stop")
        while True:
            try:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(line)
                    f.write(line + "\n")
                    f.flush()
            except KeyboardInterrupt:
                print("Done")
                break
