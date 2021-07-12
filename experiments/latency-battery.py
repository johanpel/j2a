#!/usr/bin/env python3
from experiment import Experiment
import os
import multiprocessing
import battery
import platform

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

battery.gen_schema("battery.as")

for n in range(0, 20, 1):
    for t in range(1, multiprocessing.cpu_count() + 1):
        num_jsons = 2 ** n
        if num_jsons >= t:
            # Arrow default implementation
            experiments.append(Experiment(threads=min(t, num_jsons),
                                          repeats=repeats,
                                          jsons=num_jsons,
                                          schema='schemas/battery.as',
                                          metrics_path="data/battery/latency/threads/metrics/arrow",
                                          latency_path="data/battery/latency/threads/latency/arrow"))

            # Custom implementation
            experiments.append(Experiment(threads=min(t, num_jsons),
                                          repeats=repeats,
                                          jsons=num_jsons,
                                          schema='schemas/battery.as',
                                          impl='custom-battery',
                                          metrics_path="data/battery/latency/threads/metrics/custom",
                                          latency_path="data/battery/latency/threads/latency/custom"))

            # FPGA implementations
            if (t == 16) and (platform.machine() == 'x86_64'):
                experiments.append(Experiment(threads=min(t, num_jsons),
                                              repeats=repeats,
                                              jsons=num_jsons,
                                              schema='schemas/battery.as',
                                              impl='opae-battery',
                                              metrics_path="data/battery/latency/threads/metrics/fpga",
                                              latency_path="data/battery/latency/threads/latency/fpga",
                                              machine='intel'))

            if (t == 16) and (platform.machine() == 'ppc64le'):
                experiments.append(Experiment(threads=min(t, num_jsons),
                                              repeats=repeats,
                                              jsons=num_jsons,
                                              schema='schemas/battery.as',
                                              impl='fpga-battery',
                                              metrics_path="data/battery/latency/threads/metrics/fpga",
                                              latency_path="data/battery/latency/threads/latency/fpga",
                                              machine='power'))

for e in experiments:
    print(e)
    print(e.cmd())

for e in experiments:
    e.run()
