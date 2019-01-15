#!/usr/bin/env python

import os, sys
import re
import tabulate

def main(args):
    opts = {}
    for a in args:
        m = re.match("([-_.a-zA-Z]+)=(.*)", a)
        if not m:
            continue
        opts[m.group(1)] = m.group(2)

    to_tabulate = bool(int(opts.get("tabulate", "1")))
    base_path = opts.get("base-path", "data")

    header_index = { "name" : 0, "seed" : 1, "total" : 2 }
    data = []
    for fname in sorted(os.listdir(base_path)):
        row = {}
        for line in open(os.path.join(base_path, fname)):
            row["name"] = fname
            m = re.match("([-_.a-zA-Z]+): (.*)", line)
            if not m:
                continue
            name = m.group(1)
            if name == "seed" or name == "total":
                value = m.group(2)
            else:
                value = re.split("\s", m.group(2))[2]
            if name not in header_index:
                i = len(header_index)
                header_index[name] = i
            row[name] = int(value)
        for name in row:
            if name == "name" or name == "seed" or name == "total":
                continue
            row[name] = float(row[name]) / row["total"]

        data.append(row)

    header = [""] * len(header_index)
    for k in header_index:
        header[header_index[k]] = k

    for i in range(len(data)):
        row = data[i]
        real_row = [""] * len(header_index)
        for k in row:
            real_row[header_index[k]] = row[k]
        data[i] = real_row

    if to_tabulate:
        sys.stdout.write(tabulate.tabulate(data, headers = header))
        sys.stdout.write("\n")
    else:
        first = True
        for i in header:
            if first:
                first = False
            else:
                sys.stdout.write(",")
            sys.stdout.write(i)
        sys.stdout.write("\n")
        for row in data:
            first = True
            for i in row:
                if first:
                    first = False
                else:
                    sys.stdout.write(",")
                sys.stdout.write(str(i))
            sys.stdout.write("\n")

if __name__ == "__main__":
    main(sys.argv[1:])
