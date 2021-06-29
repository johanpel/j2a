#!/usr/bin/env python3
from experiment import Experiment

for t in range(1, 17):
    Experiment(threads=t, repeats=8, jsons=1024 * 1024, schema='/io/battery.as',
               metrics="/io/data/throughput/threads/metrics",
               latency="/io/data/throughput/threads/latency").run()
