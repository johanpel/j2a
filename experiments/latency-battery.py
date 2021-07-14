#!/usr/bin/env python3
from experiment import Experiment, get_machine_config
import os
import multiprocessing
import battery
import argparse
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("--dry", action="store_true")
parser.add_argument("--no-fpga", action="store_true")
args = parser.parse_args()

# Dirs for overall metrics
os.makedirs("data/battery/latency/threads/metrics/arrow", exist_ok=True)
os.makedirs("data/battery/latency/threads/metrics/custom", exist_ok=True)
os.makedirs("data/battery/latency/threads/metrics/fpga", exist_ok=True)
# Dirs for latency metrics
os.makedirs("data/battery/latency/threads/latency/custom", exist_ok=True)
os.makedirs("data/battery/latency/threads/latency/arrow", exist_ok=True)
os.makedirs("data/battery/latency/threads/latency/fpga", exist_ok=True)
# Dirs for schemas
os.makedirs("schemas", exist_ok=True)

repeats = 8
experiments = []

# Determine what number of threads to use
step = 1
if get_machine_config() == 'power':
    step = 16
else:
    step = 4
threads = list(range(step, multiprocessing.cpu_count() + 1, step))
threads.insert(0, 1)

# Sweep from 1 MiB, 128 MiB, 1 GiB, 16 GiB.
input_size = [int(2 ** x / repeats) for x in [20, 27, 30, 34]]
# Sweep over all basic integer types for max. value.
max_value = [np.iinfo(x).max for x in [np.uint8, np.uint16, np.uint32, np.uint64]]
# Sweep over various number of array values.
max_num_values = [1, 8, 64, 512]

print("Input sizes          : {}".format(input_size))
print("Max values           : {}".format(max_value))
print("Max number of values : {}".format(max_num_values))
print("Thread counts        : {}".format(threads))

# Sweep value size from uint8 to uint64 number of digits.
for m in max_value:
    # Sweep number of array values between 1 and 512.
    for n in max_num_values:
        schema_file_prefix = 'm{}_n{}'.format(m, n)
        schema_file = 'schemas/battery_{}.as'.format(schema_file_prefix)
        battery.gen_schema(schema_file, value_max=m, num_values_max=n)

        for s in input_size:
            # FPGA implementations.
            if not args.no_fpga:
                if get_machine_config() == 'intel':
                    experiments.append(Experiment(threads=16,
                                                  repeats=repeats,
                                                  input_bytes=s,
                                                  schema=schema_file,
                                                  impl='opae-battery',
                                                  metrics_path="data/battery/latency/threads/metrics/fpga",
                                                  latency_path="data/battery/latency/threads/latency/fpga",
                                                  file_prefix=schema_file_prefix,
                                                  machine='intel',
                                                  hardware_parsers=16))

                if get_machine_config() == 'power':
                    experiments.append(Experiment(threads=20,
                                                  repeats=repeats,
                                                  input_bytes=s,
                                                  schema=schema_file,
                                                  impl='fpga-battery',
                                                  metrics_path="data/battery/latency/threads/metrics/fpga",
                                                  latency_path="data/battery/latency/threads/latency/fpga",
                                                  file_prefix=schema_file_prefix,
                                                  machine='power',
                                                  hardware_parsers=20))

            # CPU implementations
            for t in threads:
                # Arrow default implementation
                experiments.append(Experiment(schema=schema_file,
                                              threads=t,
                                              repeats=repeats,
                                              input_bytes=s,
                                              metrics_path="data/battery/latency/threads/metrics/arrow",
                                              latency_path="data/battery/latency/threads/latency/arrow",
                                              file_prefix=schema_file_prefix))

                # Custom implementation
                experiments.append(Experiment(threads=t,
                                              repeats=repeats,
                                              input_bytes=s,
                                              schema=schema_file,
                                              impl='custom-battery',
                                              metrics_path="data/battery/latency/threads/metrics/custom",
                                              latency_path="data/battery/latency/threads/latency/custom",
                                              file_prefix=schema_file_prefix))

if not args.dry:
    for i, e in enumerate(experiments):
        print("Progress: {}/{}\t{:.2f} %,".format(i, len(experiments), i / len(experiments) * 100))
        e.run()

print("Processed {} experiment configurations.".format(len(experiments)))
