#!/usr/bin/env python3
from experiment import Experiment, get_machine_config
import os
import multiprocessing
import tripreport_reduced as tr
import argparse
import numpy as np
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--dry", action="store_true", help="Only print parameters.")
parser.add_argument("--no-fpga", action="store_true", help="Don't run FPGA implementations.")
parser.add_argument("--no-cpu", action="store_true", help="Don't run CPU implementations.")
parser.add_argument("--no-schemas", action="store_true", help="Don't generate schemas.")
parser.add_argument("--bolson", type=str, help="How to run Bolson executable, e.g. \"./bolson\"", default=None)
parser.add_argument("--num-parsers", type=int, help="Number of parsers", default=8)
args = parser.parse_args()

# Dirs for overall metrics
os.makedirs("data/trip/latency/threads/metrics/arrow", exist_ok=True)
os.makedirs("data/trip/latency/threads/metrics/custom", exist_ok=True)
os.makedirs("data/trip/latency/threads/metrics/fpga", exist_ok=True)
# Dirs for latency metrics
os.makedirs("data/trip/latency/threads/latency/custom", exist_ok=True)
os.makedirs("data/trip/latency/threads/latency/arrow", exist_ok=True)
os.makedirs("data/trip/latency/threads/latency/fpga", exist_ok=True)
# Dirs for schemas
os.makedirs("schemas", exist_ok=True)

experiments = []

# Determine what number of threads to use
step = 1
if get_machine_config() == 'power':
    step = 16
else:
    step = 4
threads = list(range(step, multiprocessing.cpu_count() + 1, step))
threads.insert(0, 1)

repeats = 8
repeat_size_mod = [1, 1, 4, 32]

# Sweep from 16 MiB, 128 MiB, 1 GiB, 8 GiB.
input_size = [int(2 ** x) for x in [24, 27, 30, 33]]
# Sweep over all basic integer types for max. value.
max_value = [np.iinfo(x).max for x in [np.uint64]]  # [np.uint8, np.uint16, np.uint32, np.uint64]]

# Sweep value size from uint8 to uint64 number of digits.
for m in max_value:
    schema_file_prefix = 'm{}'.format(m)
    schema_file = 'schemas/trip_{}.as'.format(schema_file_prefix)
    if not args.no_schemas:
        tr.gen_schema(schema_file, value_max=m)

    for i, s in enumerate(input_size):
        # FPGA implementations.
        if not args.no_fpga:
            if get_machine_config() == 'intel':
                experiments.append(Experiment(threads=1,
                                              repeats=repeats,
                                              json_bytes=s,
                                              repeat_size_mod=repeat_size_mod[i],
                                              # need to allocate exactly 1 GiB for each input due to OPAE limitations
                                              input_bytes=args.num_parsers * (2 ** 30),
                                              schema=schema_file,
                                              impl='opae-trip',
                                              metrics_path="data/trip/latency/threads/metrics/fpga",
                                              latency_path="data/trip/latency/threads/latency/fpga",
                                              file_prefix=schema_file_prefix,
                                              machine='intel',
                                              hardware_parsers=args.num_parsers,
                                              bolson=args.bolson))

            if get_machine_config() == 'power':
                experiments.append(Experiment(threads=1,
                                              repeats=repeats,
                                              input_bytes=s,
                                              repeat_size_mod=repeat_size_mod[i],
                                              schema=schema_file,
                                              impl='fpga-trip',
                                              metrics_path="data/trip/latency/threads/metrics/fpga",
                                              latency_path="data/trip/latency/threads/latency/fpga",
                                              file_prefix=schema_file_prefix,
                                              machine='power',
                                              hardware_parsers=8,
                                              bolson=args.bolson))

        # CPU implementations
        if not args.no_cpu:
            for t in threads:
                # Arrow default implementation
                experiments.append(Experiment(schema=schema_file,
                                              threads=t,
                                              repeats=repeats,
                                              input_bytes=s,
                                              repeat_size_mod=repeat_size_mod[i],
                                              metrics_path="data/trip/latency/threads/metrics/arrow",
                                              latency_path="data/trip/latency/threads/latency/arrow",
                                              file_prefix=schema_file_prefix,
                                              bolson=args.bolson))

                # Custom implementation
                experiments.append(Experiment(threads=t,
                                              repeats=repeats,
                                              input_bytes=s,
                                              repeat_size_mod=repeat_size_mod[i],
                                              schema=schema_file,
                                              impl='custom-trip',
                                              metrics_path="data/trip/latency/threads/metrics/custom",
                                              latency_path="data/trip/latency/threads/latency/custom",
                                              file_prefix=schema_file_prefix,
                                              bolson=args.bolson))

if not args.dry:
    for i, e in enumerate(experiments):
        print("Progress: {}/{}\t{:.2f} %,".format(i, len(experiments), i / len(experiments) * 100))
        sys.stdout.flush()
        e.run()
else:
    for i, e in enumerate(experiments):
        print(e.cmd())

print("Processed {} experiment configurations.".format(len(experiments)))

print("Input sizes (MiB)    : {}".format([x / (2 ** 20) for x in input_size]))
print("Max values           : {}".format(max_value))
print("Thread counts        : {}".format(threads))
