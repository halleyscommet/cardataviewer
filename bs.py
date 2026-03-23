import re
import serial
import time

def replay(filename, port='/dev/cu.usbmodem1101', baud=115200):
    frames = []
    with open(filename) as f:
        for line in f:
            m = re.match(r'(\d+)\s+\[(0x[0-9A-Fa-f]+)\]\s+8\s+\[([0-9A-Fa-f\s]+)\]', line)
            if m:
                timestamp = int(m.group(1))
                pid = int(m.group(2), 16)
                data = [int(x, 16) for x in m.group(3).split()]
                frames.append((timestamp, pid, data))

    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(2)  # wait for Arduino reset

    prev_ts = frames[0][0]
    for ts, pid, data in frames:
        print(f"{ts=}, {pid=}, {data=}")
        delay = (ts - prev_ts) / 1000.0  # ms to seconds
        time.sleep(max(0, delay))
        prev_ts = ts
        # send as: PID_HIGH PID_LOW D0 D1 D2 D3 D4 D5 D6 D7 newline
        line = f"{pid:04X} {' '.join(f'{b:02X}' for b in data)}\n"
        ser.write(line.encode())

replay('revved.txt', port='/dev/cu.usbmodem1101')  # change port as needed
