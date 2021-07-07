#include <fmt/core.h>
#include <putong/putong.h>

#include <CLI/CLI.hpp>
#include <ostream>
#include <vector>

#include "arrow.h"
#include "battery.h"
// #include "trip.h"
#include "utils.h"

void PrintHeader(std::ostream* o) {
  *o << "framework,";
  *o << "api,";
  *o << "allocated,";
  *o << "num_jsons,";
  *o << "bytes_in,";
  *o << "bytes_out,";
  *o << "time_alloc,";
  *o << "time_parse,";
  *o << "time_walk,";
  *o << "max_values";
  *o << '\n';
}

void PrintResult(std::ostream* o, const BatteryParserWorkload& in, const BatteryParserResult& out) {
  *o << out.framework << ",";
  *o << out.api << ",";
  *o << (out.output_pre_allocated ? "true" : "false") << ",";
  *o << in.num_jsons << ",";
  *o << in.num_bytes << ",";
  *o << out.num_bytes << ",";
  for (const auto& s : out.timer.seconds()) {
    *o << s << ",";
  }
  *o << in.max_array_size;
  *o << std::endl;
}

// void PrintResult(std::ostream* o, const TripParserWorkload& in, const TripParserResult& out) {
//   *o << out.framework << ",";
//   *o << out.api << ",";
//   *o << (out.output_pre_allocated ? "true" : "false") << ",";
//   *o << in.num_jsons << ",";
//   *o << in.num_bytes << ",";
//   *o << out.num_bytes << ",";
//   for (const auto& s : out.timer.seconds()) {
//     *o << s << ",";
//   }
//   *o << "null";
//   *o << std::endl;
// }

// duplicate inputs for each implementation, we don't want the data to come from the
// same location to prevent caching benefit from subsequent implementations cause this
// will skew the results.

#define JSONTEST_BENCH(X)                        \
  {                                              \
    inputs.push_back(workload);                  \
    outputs.push_back(X);                        \
    inputs.back().Finish();                      \
    assert(outputs[ref].Equals(outputs.back())); \
  }

auto battery_bench(const size_t approx_size, const size_t values_end, const std::string& output_file,
                   const bool with_minified) {
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
    fmt::print(
        "Schema: battery, experiment: ({:2}/{:2}), max. voltage values: {:4}, JSONs: {:8}, Size: {:.2f} MiB, Generated in: "
        "{:.2e} s. ",
        iv + 1, values.size(), values[iv], workload.num_jsons,
        static_cast<double>(workload.bytes.size()) / ScaleMultiplier(Scale::Mi), t_gen.seconds());

    // Run experiments
    std::cout << "simdjson " << std::flush;
    inputs.push_back(workload);
    outputs.push_back(SimdBatteryParse0(inputs.back()));
    auto ref = outputs.size() - 1;
    inputs.back().Finish();

    size_t expected_values = outputs.back().num_values;
    size_t expected_offsets = outputs.back().num_offsets;

    JSONTEST_BENCH(SimdBatteryParse1(inputs.back(), expected_values, expected_offsets));
    JSONTEST_BENCH(SimdBatteryParse2(inputs.back(), expected_values, expected_offsets));

    std::cout << "RapidJSON " << std::flush;
    JSONTEST_BENCH(RapidBatteryParse0(inputs.back()));
    JSONTEST_BENCH(RapidBatteryParse1(inputs.back()));
    JSONTEST_BENCH(RapidBatteryParse2(inputs.back()));
    JSONTEST_BENCH(RapidBatteryParse3(inputs.back(), expected_values, expected_offsets));

    // custom parsing functions
    std::cout << "Custom " << std::flush;
    if (with_minified) JSONTEST_BENCH(STLParseBattery0(inputs.back(), expected_values, expected_offsets));
    if (with_minified) JSONTEST_BENCH(STLParseBattery1(inputs.back()));
    JSONTEST_BENCH(STLParseBattery2(inputs.back()));

    // parser generators
    std::cout << "ANTLR4 " << std::flush;
    JSONTEST_BENCH(ANTLRBatteryParse0(inputs.back()));

    // parser combinators
    std::cout << "Spirit " << std::flush;
    if (with_minified) JSONTEST_BENCH(SpiritBatteryParse0(inputs.back()));
    JSONTEST_BENCH(SpiritBatteryParse1(inputs.back()));

    // cuDF (GPU)
    std::cout << "cuDF " << std::flush;
    JSONTEST_BENCH(cuDFBatteryParse(inputs.back()));

    std::cout << std::endl;
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

// auto trip_bench(const size_t approx_size, const std::string& output_file, const bool with_minified) {
//   std::vector<TripParserWorkload> inputs;
//   std::vector<TripParserResult> outputs;

//   putong::Timer t_gen(true);
//   // create workload
//   auto schema = schema_trip();
//   auto workload = GenerateTripParserWorkload(*schema, approx_size, false, simdjson::SIMDJSON_PADDING);

//   t_gen.Stop();
//   fmt::print("Schema: trip, JSONs: {:8}, Size: {:.2f} MiB, Generated in: {:.2e} s. ", workload.num_jsons,
//              static_cast<double>(workload.bytes.size()) / ScaleMultiplier(Scale::Mi), t_gen.seconds());

//   // Run experiments
//   std::cout << "simdjson " << std::flush;
//   inputs.push_back(workload);
//   outputs.push_back(SimdTripParse0(inputs.back()));
//   auto ref = outputs.size() - 1;
//   inputs.back().Finish();

//   size_t expected_rows = outputs.back().batch->num_rows();
//   size_t expected_ts_values =
//       std::dynamic_pointer_cast<arrow::StringArray>(outputs.back().batch->GetColumnByName("timestamp"))->total_values_length();

//   JSONTEST_BENCH(SimdTripParse1(inputs.back(), expected_rows, expected_ts_values));
//   // JSONTEST_BENCH(SimdBatteryParse2(inputs.back(), expected_values, expected_offsets));

//   std::cout << "RapidJSON " << std::flush;
//   // JSONTEST_BENCH(RapidBatteryParse0(inputs.back()));
//   // JSONTEST_BENCH(RapidBatteryParse1(inputs.back()));
//   // JSONTEST_BENCH(RapidBatteryParse2(inputs.back()));
//   // JSONTEST_BENCH(RapidBatteryParse3(inputs.back(), expected_values, expected_offsets));

//   // custom parsing functions
//   std::cout << "Custom " << std::flush;
//   // if (with_minified) JSONTEST_BENCH(STLParseBattery0(inputs.back(), expected_values, expected_offsets));
//   // if (with_minified) JSONTEST_BENCH(STLParseBattery1(inputs.back()));
//   // JSONTEST_BENCH(STLParseBattery2(inputs.back()));

//   // parser generators
//   std::cout << "ANTLR4 " << std::flush;
//   // JSONTEST_BENCH(ANTLRBatteryParse0(inputs.back()));

//   // parser combinators
//   std::cout << "Spirit " << std::flush;
//   // if (with_minified) JSONTEST_BENCH(SpiritBatteryParse0(inputs.back()));
//   // JSONTEST_BENCH(SpiritBatteryParse1(inputs.back()));

//   std::cout << std::endl;

//   std::ostream* o = &std::cout;
//   std::ofstream f;

//   if (!output_file.empty()) {
//     f.open(output_file);
//     o = &f;
//   }

//   PrintHeader(o);
//   for (int i = 0; i < inputs.size(); i++) {
//     PrintResult(o, inputs[i], outputs[i]);
//   }

//   return EXIT_SUCCESS;
// }

auto main(int argc, char** argv) -> int {
  size_t approx_size;
  size_t values_end;
  std::string output_file;
  bool with_minified;

  CLI::App app{"JSON parsing benchmarks."};
  app.add_option("-s", approx_size, "Approximate size in B of each raw JSON dataset.")->default_val(1);
  app.add_option("-o", output_file, "CSV output file. If not set, print to stdout.");
  app.add_option("--with_minified", with_minified, "Include implementations that assume minified JSONs.")->default_val(false);

  auto* battery = app.add_subcommand("battery");
  auto* trip = app.add_subcommand("trip");
  app.require_subcommand();

  battery->add_option("v,-v", values_end, "Highest number of max battery values to sweep to.")->default_val(1);

  CLI11_PARSE(app, argc, argv);

  if (battery->parsed()) {
    battery_bench(approx_size, values_end, output_file, with_minified);
  }
  // if (trip->parsed()) {
  //   trip_bench(approx_size, output_file, with_minified);
  // }
}
