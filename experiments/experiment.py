import os


class Experiment:
    #base = 'docker run --rm -it -u $(id -u):$(id -g) --privileged -v `pwd`:/io bolson /src/bolson bench convert'
    base = 'docker run --rm -it --privileged -v `pwd`:/io bolson /src/bolson bench convert'

    def __init__(self, schema, threads=1, jsons=1, repeats=1, metrics='/io/data',
                 latency='/io/data', impl='arrow'):
        self.metrics = metrics
        self.latency = latency
        self.repeats = repeats
        self.threads = threads
        self.jsons = jsons
        self.schema = schema
        self.impl = impl

    def __str__(self):
        return 'Experiment{repeats: {}, threads: {}, jsons: {}, impl:{}}'.format(self.repeats, self.threads, self.jsons, self.impl)

    def run(self):
        command = self.base + ' ' + ' '.join([
            '--metrics {}/metrics_t{}_n{}_r{}.csv'.format(self.metrics, self.threads, self.jsons, self.repeats),
            '--latency {}/latency_t{}_n{}_r{}.csv'.format(self.latency, self.threads, self.jsons, self.repeats),
            '--repeats {}'.format(self.repeats),
            '--threads {}'.format(self.threads),
            '--num-jsons {}'.format(self.jsons),
            '--arrow-buf-cap {}'.format(128 * 1024 * 1024),
            '--parser {}'.format(self.impl),
            self.schema])

        print(command)

        os.system(command)
