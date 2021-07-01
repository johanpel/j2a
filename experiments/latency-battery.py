#!/usr/bin/env python3
from experiment import Experiment
import os
import multiprocessing

# Dirs for overall metrics
os.makedirs("data/battery/latency/threads/metrics/arrow", exist_ok=True)
os.makedirs("data/battery/latency/threads/metrics/custom", exist_ok=True)
os.makedirs("data/battery/latency/threads/metrics/fpga", exist_ok=True)
# Dirs for latency metrics
os.makedirs("data/battery/latency/threads/latency/custom", exist_ok=True)
os.makedirs("data/battery/latency/threads/latency/arrow", exist_ok=True)
os.makedirs("data/battery/latency/threads/latency/fpga", exist_ok=True)

repeats = 32
experiments = []

for n in range(0, 20, 1):
    for t in range(1, multiprocessing.cpu_count() + 1):
        num_jsons = 2**n
        if num_jsons >= t:
            experiments.append(Experiment(threads=min(t, num_jsons),
                       repeats=repeats,
                       jsons=num_jsons,
                       schema='/io/battery.as',
                       metrics="/io/data/battery/latency/threads/metrics/arrow",
                       latency="/io/data/battery/latency/threads/latency/arrow"))

            experiments.append(Experiment(threads=min(t, num_jsons),
                                          repeats=repeats,
                                          jsons=num_jsons,
                                          schema='/io/battery.as',
                                          impl='custom-battery',
                                          metrics="/io/data/battery/latency/threads/metrics/custom",
                                          latency="/io/data/battery/latency/threads/latency/custom"))

            if t == 16:
                experiments.append(Experiment(threads=min(t, num_jsons),
                           repeats=repeats,
                           jsons=num_jsons,
                           schema='/io/battery.as',
                           impl='opae-battery',
                           metrics="/io/data/battery/latency/threads/metrics/fpga",
                           latency="/io/data/battery/latency/threads/latency/fpga"))

for e in experiments:
    print(e)

for e in experiments:
    e.run()
