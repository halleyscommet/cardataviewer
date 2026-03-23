import sys
from collections import defaultdict

def parse_log(filename):
    frames = defaultdict(list)
    with open(filename, encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                if line[0].isdigit():
                    line = line.split(" ", 1)[1]
                if not line.startswith("["):
                    continue
                id_part = line.split("]")[0].strip("[")
                data_part = line.split("[")[2].strip("]")
                bytes_ = [int(b, 16) for b in data_part.split()]
                frames[id_part].append(bytes_)
            except:
                continue
    return frames

def avg_bytes(frames):
    averages = {}
    for can_id, frame_list in frames.items():
        num_bytes = len(frame_list[0])
        avgs = []
        for i in range(num_bytes):
            vals = [f[i] for f in frame_list if i < len(f)]
            avgs.append(sum(vals) / len(vals))
        averages[can_id] = avgs
    return averages

if len(sys.argv) != 3:
    print("Usage: python compare.py idle_log.txt revved_log.txt")
    sys.exit(1)

idle_frames = parse_log(sys.argv[1])
revved_frames = parse_log(sys.argv[2])

idle_avgs = avg_bytes(idle_frames)
revved_avgs = avg_bytes(revved_frames)

# only compare IDs present in both logs
common_ids = set(idle_avgs.keys()) & set(revved_avgs.keys())

print(f"{'ID':<8} {'Byte':<6} {'Idle avg':>10} {'Revved avg':>12} {'Delta':>10}  {'':>4}")
print("-" * 55)

results = []
for can_id in common_ids:
    idle = idle_avgs[can_id]
    revved = revved_avgs[can_id]
    for i in range(min(len(idle), len(revved))):
        delta = revved[i] - idle[i]
        if abs(delta) > 5:  # ignore noise, only show meaningful shifts
            results.append((abs(delta), can_id, i, idle[i], revved[i], delta))

results.sort(reverse=True)

for _, can_id, byte_i, idle_val, revved_val, delta in results:
    arrow = "^^" if delta > 0 else "vv"
    print(f"{can_id:<8} Byte {byte_i}  {idle_val:>10.1f} {revved_val:>12.1f} {delta:>+10.1f}  {arrow}")
