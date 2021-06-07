#!/usr/bin/env python3
from experiment import Experiment
import os

os.makedirs("data/latency/threads/metrics", exist_ok=True)
os.makedirs("data/latency/threads/latency", exist_ok=True)

for n in range(0, 20, 1):
    for t in range(1, 17):
        Experiment(threads=min(t, 2 ** n),
                   repeats=64,
                   jsons=2 ** n,
                   schema='io/battery.as',
                   metrics="io/data/latency/threads/metrics/cpu",
                   latency="io/data/latency/threads/latency/cpu").run()

        Experiment(threads=min(t, 2 ** n),
                   repeats=64,
                   jsons=2 ** n,
                   schema='io/battery.as',
                   impl='opae-battery',
                   metrics="io/data/latency/threads/metrics/fpga",
                   latency="io/data/latency/threads/latency/fpga").run()
