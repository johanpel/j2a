#include <fmt/core.h>
#include <putong/putong.h>

#include <CLI/CLI.hpp>
#include <fstream>
#include <ostream>
#include <vector>

#include "arrow.h"
#include "battery.h"
#include "utils.h"

void PrintHeader(std::ostream* o) {
  *o << "framework,";
  *o << "api,";
  *o << "allocated,";
  *o << "max_values,";
  *o << "num_jsons,";
  *o << "bytes_in,";
  *o << "bytes_out,";
  *o << "time";
  *o << '\n';
}

void PrintResult(std::ostream* o, const BatteryParserWorkload& in, const BatteryParserResult& out) {
  *o << out.framework << ",";
  *o << out.api << ",";
  *o << (out.output_pre_allocated ? "true" : "false") << ",";
  *o << in.max_array_size << ",";
  *o << in.num_jsons << ",";
  *o << in.num_bytes << ",";
  *o << out.num_bytes << ",";
  *o << Sum(out.timer.seconds()) << std::endl;
}

auto main(int argc, char** argv) -> int {
  size_t approx_size;
  size_t values_end;
  std::string output_file;

  CLI::App app{"App description"};
  app.add_option("n,-n", approx_size, "Approximate size in B of each raw JSON dataset.")->default_val(1024 * 1024);
  app.add_option("v,-v", values_end, "Highest number of max battery values to sweep to.")->default_val(16);
  app.add_option("o,-o", output_file, "CSV output file. If not set, print to stdout.");
  CLI11_PARSE(app, argc, argv);

  if (argc == 2) {
    values_end = std::strtoul(argv[1], nullptr, 10);
  }

  std::vector<size_t> values = {};
  for (size_t i = 0; static_cast<size_t>(std::pow(2, i)) <= values_end; i++) {
    values.push_back(static_cast<size_t>(std::pow(2, i)));
  }

  std::vector<BatteryParserWorkload> inputs;
  std::vector<BatteryParserResult> outputs;

  for (size_t iv = 0; iv < values.size(); iv++) {
    size_t max_num_values = values[iv];
    putong::Timer t_gen(true);
    // create workload
    auto schema = schema_battery(max_num_values);
    auto workload = GenerateBatteryParserWorkload(*schema, approx_size, false, simdjson::SIMDJSON_PADDING);

    t_gen.Stop();
    fmt::print("{:4} ({:2}/{:2}), {:8} JSONs, {:.2f} MiB, {:.2e} s. ", values[iv], iv + 1, values.size(), workload.num_jsons,
               static_cast<double>(workload.bytes.size()) / ScaleMultiplier(Scale::Mi), t_gen.seconds());

    bool require_ws = true;

    // duplicate inputs for each implementation, we don't want the data to come from the
    // same location to prevent caching benefit from subsequent implementations cause this
    // will skew the results.

#define JSONTEST_BENCH(X)                         \
  {                                               \
    inputs.push_back(workload);                   \
    outputs.push_back(X);                         \
    inputs.back().Finish();                       \
    assert(outputs[ref].IsEqual(outputs.back())); \
  }

    // Run experiments
    std::cout << "simdjson.. " << std::flush;
    inputs.push_back(workload);
    outputs.push_back(SimdBatteryParse0(inputs.back()));
    auto ref = outputs.size() - 1;
    inputs.back().Finish();

    size_t expected_values = outputs.back().num_values;
    size_t expected_offsets = outputs.back().num_offsets;

    JSONTEST_BENCH(SimdBatteryParse1(inputs.back(), expected_values, expected_offsets));
    JSONTEST_BENCH(SimdBatteryParse2(inputs.back(), expected_values, expected_offsets));

    std::cout << "RapidJSON.. " << std::flush;
    JSONTEST_BENCH(RapidBatteryParse0(inputs.back()));
    JSONTEST_BENCH(RapidBatteryParse1(inputs.back()));
    JSONTEST_BENCH(RapidBatteryParse2(inputs.back()));
    JSONTEST_BENCH(RapidBatteryParse3(inputs.back(), expected_values, expected_offsets));

    // custom parsing functions
    std::cout << "Custom.. " << std::flush;
    if (!require_ws) JSONTEST_BENCH(STLParseBattery0(inputs.back(), expected_values, expected_offsets));
    if (!require_ws) JSONTEST_BENCH(STLParseBattery1(inputs.back()));
    JSONTEST_BENCH(STLParseBattery2(inputs.back()));

    // parser generators
    // std::cout << "ANTLR4.. " << std::flush;
    // JSONTEST_BENCH(ANTLRBatteryParse0(inputs.back()));

    // parser combinators
    std::cout << "Spirit.." << std::flush;
    if (!require_ws) JSONTEST_BENCH(SpiritBatteryParse0(inputs.back()));
    JSONTEST_BENCH(SpiritBatteryParse1(inputs.back()));

    std::cout << std::endl;
    // std::cout << std::string(workload.bytes.data(), workload.bytes.size()) << std::endl;
  }

  std::ostream* o = &std::cout;
  std::ofstream f;

  if (!output_file.empty()) {
    f.open(output_file);
    o = &f;
  }

  PrintHeader(o);
  for (int i = 0; i < inputs.size(); i++) {
    PrintResult(o, inputs[i], outputs[i]);
  }

  return EXIT_SUCCESS;
}
