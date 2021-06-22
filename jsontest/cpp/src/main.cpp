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

void PrintResult(std::ostream* o, const BatteryParserWorkload& in,
                 const BatteryParserResult& out) {
  *o << out.framework << ",";
  *o << out.api << ",";
  *o << (out.output_pre_allocated ? "true" : "false") << ",";
  *o << in.max_array_size << ",";
  *o << in.num_jsons << ",";
  *o << DataSizeOf(in.bytes) << ",";
  *o << DataSizeOf(out.values) << ",";
  *o << Sum(out.timer.seconds()) << std::endl;
}

auto main(int argc, char** argv) -> int {
  size_t num_jsons;
  size_t values_end;
  std::string output_file;

  CLI::App app{"App description"};
  app.add_option("n,-n", num_jsons, "Number of JSONs.")->default_val(1024);
  app.add_option("v,-v", values_end, "Highest number of max battery values to sweep to.")
      ->default_val(16);
  app.add_option("o,-o", output_file, "CSV output file. If not set, print to stdout.");
  CLI11_PARSE(app, argc, argv);

  if (argc == 2) {
    values_end = std::strtoul(argv[1], nullptr, 10);
  }
  size_t num_implementations = 11;

  std::vector<size_t> values = {};
  for (size_t i = 0; static_cast<size_t>(std::pow(2, i)) <= values_end; i++) {
    values.push_back(static_cast<size_t>(std::pow(2, i)));
  }

  std::cout << "Max. number of voltage values: ";
  for (const auto& mv : values) {
    std::cout << mv << ", ";
  }
  std::cout << std::endl;

  std::vector<BatteryParserWorkload> inputs;
  std::vector<BatteryParserResult> outputs;

  for (size_t iv = 0; iv < values.size(); iv++) {
    size_t max_num_values = values[iv];
    putong::Timer t_gen(true);
    // create workload
    auto schema = schema_battery(max_num_values);
    auto workload = GenerateBatteryParserWorkload(*schema, num_jsons, false,
                                                  simdjson::SIMDJSON_PADDING);

    t_gen.Stop();
    std::cout << "Generate data : " << t_gen.seconds() << " s." << std::endl;

    // duplicate inputs for each implementation, we don't want the data to come from the
    // same location to prevent caching benefit from subsequent implementations cause this
    // will skew the results.
    std::cout << "Duplicating input data..." << std::endl;
    for (size_t i = 0; i < num_implementations; i++) {
      inputs.push_back(workload);
    }

    // Run experiments
    std::cout << "Running simdjson impls..." << std::endl;
    size_t off = num_implementations * iv;
    outputs.push_back(SimdBatteryParse0(inputs[off + 0]));
    size_t expected_out_size = outputs[off + 0].values.size();
    outputs.push_back(SimdBatteryParse1(inputs[off + 1], expected_out_size));
    outputs.push_back(SimdBatteryParse2(inputs[off + 2], expected_out_size));

    std::cout << "Running RapidJSON impls..." << std::endl;
    outputs.push_back(RapidBatteryParse0(inputs[off + 3]));
    outputs.push_back(RapidBatteryParse1(inputs[off + 4]));
    outputs.push_back(RapidBatteryParse2(inputs[off + 5]));
    outputs.push_back(RapidBatteryParse3(inputs[off + 6], expected_out_size));

    // custom parsing functions
    std::cout << "Running custom STL impls..." << std::endl;
    outputs.push_back(STLParseBattery0(inputs[off + 7], expected_out_size));
    outputs.push_back(STLParseBattery1(inputs[off + 8]));
    // outputs.push_back(STLParseBattery2(inputs[off + 9]));

    // parser generators
    std::cout << "Running ANTLR4 impls..." << std::endl;
    outputs.push_back(ANTLRBatteryParse0(inputs[off + 9]));

    // parser combinators
    std::cout << "Running Spirit impls..." << std::endl;
    outputs.push_back(SpiritBatteryParse0(inputs[off + 10]));

    // Clear inputs and outputs after parsing to save memory.
    for (size_t i = 0; i < num_implementations; i++) {
      inputs[off + i].Finish();
      outputs[off + i].Finish();
    }
  }

  std::cout << "Number of input data sets: " << inputs.size() << std::endl;
  std::cout << "Number of output data sets: " << outputs.size() << std::endl;
  std::cout << "Number of max values: " << values.size() << std::endl;

  std::ostream* o = &std::cout;
  std::ofstream f;

  if (!output_file.empty()) {
    f.open(output_file);
    o = &f;
  }

  PrintHeader(o);
  for (int i = 0; i < num_implementations * values.size(); i++) {
    PrintResult(o, inputs[i], outputs[i]);
  }

  return EXIT_SUCCESS;
}
