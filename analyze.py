import sys
from collections import defaultdict

TARGET_ID = "0x232"  # change to whatever ID you want to analyze

byte_values = defaultdict(set)

with open("idle.txt", encoding="utf-8", errors="ignore") as f:
    for line in f:
        line = line.strip()
        if not line.startswith("["):
            continue
        try:
            id_part = line.split("]")[0].strip("[")
            data_part = line.split("[")[2].strip("]")
            if id_part.lower() != TARGET_ID.lower():
                continue
            bytes_ = data_part.split()
            for i, b in enumerate(bytes_):
                byte_values[i].add(b)
        except:
            continue

print(f"Byte variance for ID {TARGET_ID}:")
for i, vals in sorted(byte_values.items()):
    print(f"  Byte {i}: {len(vals)} unique values — {sorted(vals)[:10]}{'...' if len(vals) > 10 else ''}")
