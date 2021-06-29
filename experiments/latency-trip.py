#!/usr/bin/env python3
from experiment import Experiment
import os
import multiprocessing

os.makedirs("data/trip/latency/threads/metrics/cpu", exist_ok=True)
os.makedirs("data/trip/latency/threads/latency/cpu", exist_ok=True)
os.makedirs("data/trip/latency/threads/metrics/fpga", exist_ok=True)
os.makedirs("data/trip/latency/threads/latency/fpga", exist_ok=True)

repeats = 32
experiments = []

for n in range(0, 20, 1):
    for t in range(1, multiprocessing.cpu_count() + 1):
        num_jsons = 2**n
        if num_jsons >= t:
            experiments.append(Experiment(threads=min(t, num_jsons),
                       repeats=repeats,
                       jsons=num_jsons,
                       schema='/io/trip.as',
                       metrics="/io/data/trip/latency/threads/metrics/cpu",
                       latency="/io/data/trip/latency/threads/latency/cpu"))

            if t == 1:
                experiments.append(Experiment(threads=min(t, num_jsons),
                           repeats=repeats,
                           jsons=num_jsons,
                           schema='/io/trip.as',
                           impl='opae-trip',
                           metrics="/io/data/trip/latency/threads/metrics/fpga",
                           latency="/io/data/trip/latency/threads/latency/fpga"))

for e in experiments:
    print(e)

for e in experiments:
    e.run()
