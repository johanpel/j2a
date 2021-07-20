# Experiments related to JSON parsing and conversion to Apache Arrow

## JSON to Arrow conversion in software

This project contains a microbenchmark for JSON-to-Arrow conversion for specific Arrow target schemas, with various
frameworks, including:

* RapidJSON
* simdjson
* Boost Spirit.X3
* cuDF
* Hand-written

These are found in the `jsontest` folder.

## FPGA accelerated application benchmarks

This project also contains Python scripts that accompany the [Bolson](https://github.com/teratide/bolson) project to
measure integrated performance of FPGA accelerated parsing. These scripts are found in the `experiments` folder. The
scripts to plot the results are found in the `plots` folder.