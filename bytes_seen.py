from collections import defaultdict

byte_values = defaultdict(lambda: defaultdict(int))
frame_counts = defaultdict(int)

with open("idle.txt", encoding="utf-8", errors="ignore") as f:
    for line in f:
        line = line.strip()
        if not line.startswith("[") and not (line[0].isdigit() if line else False):
            continue
        try:
            # strip millis if present
            if line[0].isdigit():
                line = line.split(" ", 1)[1]
            id_part = line.split("]")[0].strip("[")
            data_part = line.split("[")[2].strip("]")
            bytes_ = data_part.split()
            frame_counts[id_part] += 1
            for i, b in enumerate(bytes_):
                byte_values[id_part][i] += 1 if b not in byte_values[id_part] else 0
                byte_values[id_part][f"{i}_vals"] = byte_values[id_part].get(f"{i}_vals", set())
                byte_values[id_part][f"{i}_vals"].add(b)
        except:
            continue

for can_id in sorted(frame_counts.keys()):
    print(f"\nID {can_id} — {frame_counts[can_id]} frames")
    i = 0
    while f"{i}_vals" in byte_values[can_id]:
        vals = byte_values[can_id][f"{i}_vals"]
        print(f"  Byte {i}: {len(vals):3d} unique values", end="")
        if len(vals) <= 6:
            print(f"  {sorted(vals)}", end="")
        print()
        i += 1
