import os
import platform


def get_machine_config():
    if platform.machine() == 'ppc64le':
        return 'power'
    elif platform.machine() == 'x86_64':
        return 'intel'
    else:
        raise Exception("Unsupported machine.")


class Experiment:
    def __init__(self, schema, threads=1, input_bytes=1, json_bytes=None, repeats=1, metrics_path='data',
                 latency_path='data', impl='arrow', hardware_parsers=16, machine='auto', file_prefix='', bolson=None):
        self.metrics_path = metrics_path
        self.latency_path = latency_path
        self.file_prefix = file_prefix
        self.repeats = repeats
        self.threads = threads
        self.input_bytes = input_bytes
        self.json_bytes = json_bytes
        self.schema = schema
        self.impl = impl
        self.hardware_parsers = hardware_parsers
        self.paths_prefix = ""

        if bolson is None:
            if machine == 'auto':
                machine = get_machine_config()

            if machine == "intel":
                # On the intel machine we run in a docker container
                self.bolson = 'docker run --rm -it --privileged -v `pwd`:/io bolson'
                self.paths_prefix = "/io/"
            elif machine == "power":
                # On the POWER machine we don't run in docker.
                self.bolson = 'bolson'
            else:
                raise ValueError("Expected \"power\" or \"intel\", got {}".format(machine))
        else:
            self.bolson = bolson

    def __str__(self):
        return 'Experiment{{repeats: {}, threads: {}, json_bytes: {}, impl:{}}}'.format(self.repeats, self.threads,
                                                                                        self.json_bytes, self.impl)

    def cmd(self):
        return self.bolson + ' bench convert ' + ' '.join([
            '--metrics {}{}/metrics_{}_t{}_s{}_r{}.csv'.format(self.paths_prefix, self.metrics_path,
                                                               self.file_prefix, self.threads,
                                                               self.json_bytes,
                                                               self.repeats),
            '--latency {}{}/latency_{}_t{}_s{}_r{}.csv'.format(self.paths_prefix, self.latency_path,
                                                               self.file_prefix, self.threads,
                                                               self.json_bytes,
                                                               self.repeats),
            '--repeats {}'.format(self.repeats),
            '--threads {}'.format(self.threads),
            '--input-buffers-capacity {}'.format(self.input_bytes),
            # use input bytes if no json bytes are supplied
            '--total-json-bytes {}'.format(self.json_bytes if self.json_bytes else self.input_bytes),
            '--parser {}'.format(self.impl),
            '--trip-num-parsers {}'.format(self.hardware_parsers),
            '--battery-num-parsers {}'.format(self.hardware_parsers),
            '--fpga-battery-num-parsers {}'.format(self.hardware_parsers),
            '--parse-only',
            '--input {}{}'.format(self.paths_prefix, self.schema)])

    def run(self):
        print(self.cmd())
        os.system(self.cmd())
