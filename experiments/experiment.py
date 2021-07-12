import os


class Experiment:

    def __init__(self, schema, threads=1, jsons=1, repeats=1, metrics_path='data',
                 latency_path='data', impl='arrow', trip_parsers=3, battery_parsers=16, machine="intel"):
        self.metrics = metrics_path
        self.latency = latency_path
        self.repeats = repeats
        self.threads = threads
        self.jsons = jsons
        self.schema = schema
        self.impl = impl
        self.trip_parsers = trip_parsers
        self.battery_parsers = battery_parsers
        if machine == "intel":
            # On the intel machine we run in a docker container
            self.base = 'docker run --rm -it --privileged -v `pwd`:/io bolson /src/bolson bench convert'
            self.paths_prefix = "/io/"
        elif machine == "power":
            # On the POWER machine we don't run in docker.
            self.base = 'bolson bench convert'
            self.paths_prefix = ""
        else:
            raise ValueError("Expected \"power\" or \"intel\", got {}".format(machine))

    def __str__(self):
        return 'Experiment{{repeats: {}, threads: {}, jsons: {}, impl:{}}}'.format(self.repeats, self.threads,
                                                                                   self.jsons, self.impl)

    def cmd(self):
        return self.base + ' ' + ' '.join([
            '--metrics {}{}/metrics_t{}_n{}_r{}.csv'.format(self.paths_prefix, self.metrics, self.threads, self.jsons,
                                                            self.repeats),
            '--latency {}{}/latency_t{}_n{}_r{}.csv'.format(self.paths_prefix, self.latency, self.threads, self.jsons,
                                                            self.repeats),
            '--repeats {}'.format(self.repeats),
            '--threads {}'.format(self.threads),
            '--num-jsons {}'.format(self.jsons),
            '--arrow-buf-cap {}'.format(1024 * 1024 * 1024),
            '--custom-battery-buf-cap {}'.format(1024 * 1024 * 1024),
            '--custom-battery-pre-alloc-offsets {}'.format(1024 * 1024),
            '--custom-battery-pre-alloc-values {}'.format(int(1024 * 1024 * 1024 / 8)),
            '--custom-trip-buf-cap {}'.format(1024 * 1024 * 1024),
            '--custom-trip-pre-alloc-records {}'.format(1024 * 1024),
            '--custom-battery-pre-alloc-timestamp-values {}'.format(1024 * 1024 * 1024),
            '--parser {}'.format(self.impl),
            '--trip-num-parsers {}'.format(self.trip_parsers),
            '--battery-num-parsers {}'.format(self.battery_parsers),
            '{}{}'.format(self.paths_prefix, self.schema)])

    def run(self):
        print(self.cmd())
        os.system(self.cmd())
