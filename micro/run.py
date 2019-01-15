#!/usr/bin/env python

import sys, os, subprocess
import random

SOURCE_DIR = os.path.dirname(os.path.abspath(__file__))

def run(opts, algo_list, **kwargs):
    cmd = ["build/Main", "progress-report=0"]
    cmd.append("seed={0}".format(random.getrandbits(32)))

    for k in opts:
        cmd.append("{0}={1}".format(k, opts[k]))
    for a in algo_list:
        cmd.append(a)

    if "output" in kwargs:
        subprocess.check_call(cmd, stdout = open(kwargs["output"], "w"))
    else:
        subprocess.check_call(cmd)

def main(args):
    benchmarks = set(args)

    os.chdir(SOURCE_DIR)
    if not os.path.isdir(os.path.join(SOURCE_DIR, "data")):
        os.mkdir(os.path.join(SOURCE_DIR, "data"))
    subprocess.check_call("make", shell = True, stdout = open("/dev/null", "w"))

    if "seed" in os.environ:
        random.seed(int(os.environ["seed"]))

    if "rainbow" not in benchmarks:
        for i in range(10):
            # advance rng
            random.getrandbits(32)
    else:
        for i in range(10):
            output = os.path.join(SOURCE_DIR, "data", "rainbow-rw-{0}.txt".format(i))
            if os.path.exists(output):
                os.remove(output)
        for i in range(10):
            run({
                "name" : "rainbow",
                "rb.width" : 4,
                "rb.length" : 3,
                "dep-name" : "rw",
                "rw.skew" : 0.5
                },
                [ "random-walk.basic",
                  "pos.basic",
                  "pos.dep-based",
                ],
                output = os.path.join(SOURCE_DIR, "data", "rainbow-rw-{0}.txt".format(i)))

    if "double-tree" not in benchmarks:
        for i in range(10):
            # advance rng
            random.getrandbits(32)
    else:
        for i in range(10):
            output = os.path.join(SOURCE_DIR, "data", "double-tree-rw-{0}.txt".format(i))
            if (os.path.exists(output)):
                os.remove(output)
        for i in range(10):
            run({
                "name" : "double-tree",
                "dt.depth" : 3,
                "dep-name" : "rw",
                "rw.skew" : 0.1
                },
                [
                    "random-walk.basic",
                    "pos.basic",
                    "pos.dep-based",
                ],
                output = os.path.join(SOURCE_DIR, "data", "double-tree-rw-{0}.txt".format(i)))

if __name__ == "__main__":
    main(sys.argv[1:])
