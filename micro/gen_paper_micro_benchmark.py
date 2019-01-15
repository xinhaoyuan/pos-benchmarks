#!/usr/bin/env python

import sys, os, subprocess
import random

local_dir = os.path.dirname(os.path.realpath(__file__))

os.chdir(local_dir)
try:
    os.mkdir("paper-micro-bench")
except:
    pass

rng = random.Random()
rng.seed(0)

def Gen(seed, filename, opts):
    cmd = [ "build/DataGen" ]
    opts["seed"] = seed
    for k in opts:
        cmd.append("{0}={1}".format(k, opts[k]))
    subprocess.check_call(cmd, stdout = open(filename, "w"))

subprocess.check_call([ "make" ], stdout = open(os.devnull, "w"))

benchmarks_simple = bool(int(os.environ.get("BENCHMARKS_SIMPLE", "0")))

if benchmarks_simple:
    for i in range(5):
        name = "paper-micro-bench/uniform_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 3,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "4,0,4,0,4,0"
            })

    for i in range(5):
        name = "paper-micro-bench/skewed_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 3,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "2,0,4,0,6,0"
            })

    for i in range(5):
        name = "paper-micro-bench/uniform-rw_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 3,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "2,2,2,2,2,2"
            })

    for i in range(5):
        name = "paper-micro-bench/skewed-rw_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 3,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "1,1,2,2,3,3"
            })
else:
    for i in range(5):
        name = "paper-micro-bench/uniform_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 4,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "4,0,4,0,4,0,4,0"
            })

    for i in range(5):
        name = "paper-micro-bench/skewed_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 4,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "2,0,2,0,6,0,6,0"
            })

    for i in range(5):
        name = "paper-micro-bench/uniform-rw_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 4,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "2,2,2,2,2,2,2,2"
            })

    for i in range(5):
        name = "paper-micro-bench/skewed-rw_{0}".format(i)
        print(name)
        Gen(rng.getrandbits(32),
            name,
            { "name" : "rainbow",
              "rb.width" : 4,
              "rb.length" : 4,
              "dep-name" : "rwd",
              "rwd.dist" : "1,1,1,1,3,3,3,3"
            })
