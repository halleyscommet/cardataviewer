with open("idle.txt", encoding="utf-8", errors="ignore") as f:
    for line in f:
        line = line.strip()
        try:
            if line[0].isdigit():
                line = line.split(" ", 1)[1]
            if not line.startswith("[0x231]"):
                continue
            data = line.split("[")[2].strip("]").split()
            b0 = int(data[0], 16)
            b1 = int(data[1], 16)
            raw = (b1 << 8) | b0
            print(f"{raw}  →  x0.25={raw*0.25:.0f}  x0.5={raw*0.5:.0f}  /8={raw//8}")
        except:
            continue
