#include <cudf/io/json.hpp>
#include <cudf/io/types.hpp>
#include <cudf/table/table.hpp>
#include <cudf/types.hpp>

namespace gpu {

auto cuDFBatteryParse(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("cuDF", "read_json", false, data.max_array_values);
  result.timer.Start();

  cudf::io::json_reader_options read_options =
      cudf::io::json_reader_options::builder(cudf::io::source_info{data.bytes.data(), data.num_bytes});
  std::unique_ptr<cudf::table> table = cudf::io::read_json(read_options).tbl;

  result.timer.Split();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

}  // namespace gpu