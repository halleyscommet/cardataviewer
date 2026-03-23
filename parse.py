import re

def parse_rpm(input_file, output_file):
    with open(input_file) as f_in, open(output_file, 'w') as f_out:
        for line in f_in:
            m = re.match(r'(\d+)\s+\[(0x231)\]\s+8\s+\[([0-9A-Fa-f\s]+)\]', line, re.IGNORECASE)
            if m:
                timestamp = m.group(1)
                data = [int(x, 16) for x in m.group(3).split()]
                rpm = data[0] | (data[1] << 8)
                f_out.write(f"{timestamp} {rpm}\n")

parse_rpm('idle.txt', 'idle_rpm.txt')
parse_rpm('revved.txt', 'revved_rpm.txt')
