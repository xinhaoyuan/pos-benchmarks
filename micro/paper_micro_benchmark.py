#!/usr/bin/env python

import sys, os, subprocess
import random
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument("-j", type = int, dest = "nproc", default = 1)
parser.add_argument("-i", type = str, dest = "input")
parser.add_argument("-s", type = int, dest = "n_sample", default = 50000000)
parser.add_argument("--no-sample", dest = "no_sample", action = "store_true")
args = parser.parse_args()

input = open(args.input)

local_dir = os.path.dirname(os.path.realpath(__file__))

env = os.environ.copy()

if args.no_sample:
    print("no sample")
else:
    env["CALC_PCT_PARAM"] = "0 -1 {0} 0".format(args.n_sample)
    env["CALC_BPOS_SAMPLE"] = "{0} 0".format(args.n_sample)
    env["CALC_POS_SAMPLE"] = "{0} 0".format(args.n_sample)
    env["CALC_RPOS_SAMPLE"] = "{0} 0".format(args.n_sample)
    env["CALC_RAPOS_SAMPLE"] = "{0} 0".format(args.n_sample)

jobs = []

for line in input:
    case_name = line.rstrip()
    if case_name.startswith("#"):
        continue

    while len(jobs) >= args.nproc:
        idleJobIdx = None
        for idx in range(len(jobs)):
            if jobs[idx][0].poll() is not None:
                idleJobIdx = idx
                break
        if idleJobIdx is not None:
            if jobs[idleJobIdx][0].returncode != 0:
                print("job returns {0}".format(jobs[idleJobIdx][0].returncode))
            del jobs[idleJobIdx]
        else:
            time.sleep(1)

    print(case_name)
    job_input = open(case_name)
    job_output = open(case_name + ".result", "w")
    # cmd = [ os.path.join(local_dir, "build", "Calc") ]
    cmd = [ "build/Calc" ]
    p = subprocess.Popen(cmd, env = env, stdin = job_input, stdout = job_output)
    # cmd = [ "/bin/ls" ]
    # p = subprocess.Popen(cmd, env = env)
    jobs.append((p, job_input, job_output))

while len(jobs) > 0:
    idleJobIdx = None
    for idx in range(len(jobs)):
        if jobs[idx][0].poll() is not None:
            idleJobIdx = idx
            break
    if idleJobIdx is not None:
        del jobs[idleJobIdx]
    else:
        time.sleep(1)
