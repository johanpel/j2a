#include <putong/putong.h>

#include <CLI/CLI.hpp>
#include <fstream>
#include <ostream>
#include <vector>

#include "arrow.h"
#include "battery.h"
#include "utils.h"

void PrintResult(std::ostream* f, const BatteryParserWorkload& in,
                 const BatteryParserResult& out) {
  *f << out.framework << ",";
  *f << out.api << ",";
  *f << (out.output_pre_allocated ? "true" : "false") << ",";
  *f << in.max_array_size << ",";
  *f << in.num_jsons << ",";
  *f << DataSizeOf(in.bytes) << ",";
  *f << DataSizeOf(out.values) << ",";
  *f << Sum(out.timer.seconds()) << std::endl;
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

  std::ostream* o;
  if (!output_file.empty()) {
    std::ofstream f(output_file);
    o = &f;
  } else {
    o = &std::cout;
  }

  if (argc == 2) {
    values_end = std::strtoul(argv[1], nullptr, 10);
  }
  size_t num_implementations = 12;

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
    auto workload = GenerateBatteryParserWorkload(*schema, num_jsons, false, false,
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

    // json parsing libs

    std::cout << "Running simdjson impls..." << std::endl;
    outputs.push_back(SimdBatteryParse0(inputs[0]));
    size_t expected_out_size = outputs[0].values.size();
    outputs.push_back(SimdBatteryParse1(inputs[1], expected_out_size));
    outputs.push_back(SimdBatteryParse2(inputs[2], expected_out_size));

    std::cout << "Running RapidJSON impls..." << std::endl;
    outputs.push_back(RapidBatteryParse0(inputs[3]));
    outputs.push_back(RapidBatteryParse1(inputs[4]));
    outputs.push_back(RapidBatteryParse2(inputs[5]));
    outputs.push_back(RapidBatteryParse3(inputs[6], expected_out_size));

    // custom parsing functions
    std::cout << "Running custom STL impls..." << std::endl;
    outputs.push_back(STLParseBattery0(inputs[7], expected_out_size));
    outputs.push_back(STLParseBattery1(inputs[8]));
    outputs.push_back(STLParseBattery2(inputs[9]));

    // parser generators
    std::cout << "Running ANTLR4 impls..." << std::endl;
    outputs.push_back(ANTLRBatteryParse0(inputs[10]));

    // parser combinators
    std::cout << "Running Spirit impls..." << std::endl;
    outputs.push_back(SpiritBatteryParse0(inputs[11]));
  }

  std::cout << "Number of input data sets: " << inputs.size() << std::endl;
  std::cout << "Number of output data sets: " << outputs.size() << std::endl;
  std::cout << "Number of max values: " << values.size() << std::endl;

  for (int i = 0; i < num_implementations * values.size(); i++) {
    PrintResult(o, inputs[i], outputs[i]);
  }

  return EXIT_SUCCESS;
}