#pragma once

#include <arrow/api.h>
#include <putong/timer.h>
#include <rapidjson/rapidjson.h>
#include <simdjson.h>

#include <boost/spirit/home/x3.hpp>
#include <charconv>
#include <iostream>
#include <numeric>
#include <utility>

struct TripParserWorkload {
  size_t num_jsons = 1;
  size_t num_bytes = 0;
  std::vector<char> bytes;

  void Finish() {
    num_bytes = bytes.size();
    bytes = std::vector<char>();
  }
};

struct TripParserResult {
  TripParserResult(std::string framework, std::string api, bool pre_alloc)
      : framework(std::move(framework)), api(std::move(api)), output_pre_allocated(pre_alloc) {}

  // Impl. properties
  std::string framework = "null";
  std::string api = "null";
  bool output_pre_allocated = false;

  // Arrow buffers:
  std::vector<uint64_t> values;
  std::vector<int32_t> offsets;  // Arrow uses int32_t for offsets O_o

  // Statistics
  putong::SplitTimer<3> timer;
  size_t num_bytes = 0;
  size_t num_values = 0;
  size_t num_offsets = 0;
  uint64_t checksum = 0;

  void Print() const {
    std::cout << framework << " " << api << " " << output_pre_allocated << std::endl;

    std::cout << voltage_column.ToString() << std::endl;
  }

  void Finish() {
    checksum = std::accumulate(values.begin(), values.end(), 0UL) + std::accumulate(offsets.begin(), offsets.end(), 0UL);
    num_values = values.size();
    num_offsets = offsets.size();
    num_bytes = values.size() * sizeof(uint64_t) + offsets.size() * sizeof(int32_t);
    values = std::vector<uint64_t>();
    offsets = std::vector<int32_t>();
  }

  [[nodiscard]] bool IsEqual(const TripParserResult& other) const {
    if (checksum != other.checksum) {
      std::cerr << "Checksum: " << checksum << " != " << other.checksum << std::endl;
    } else if (num_offsets != other.num_offsets) {
      std::cerr << "No. offsets: " << num_offsets << " != " << other.num_offsets << std::endl;
    } else if (num_bytes != other.num_bytes) {
      std::cerr << "No. bytes: " << num_bytes << " != " << other.num_bytes << std::endl;
    } else {
      return true;
    }
    return false;
  }
};

auto schema_trip(size_t max_array_size = 16) -> std::shared_ptr<arrow::Schema> {
  static auto result = arrow::schema(
      {arrow::field("timestamp", arrow::utf8(), false), arrow::field("timezone", arrow::uint64(), false),
       arrow::field("vin", arrow::uint64(), false), arrow::field("odometer", arrow::uint64(), false),
       arrow::field("hypermiling", arrow::boolean(), false), arrow::field("avgspeed", arrow::uint64(), false),
       arrow::field("sec_in_band", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 12), false),
       arrow::field("miles_in_time_range", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 24), false),
       arrow::field("const_speed_miles_in_band", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 12),
                    false),
       arrow::field("vary_speed_miles_in_band", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 12),
                    false),
       arrow::field("sec_decel", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 10), false),
       arrow::field("sec_accel", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 10), false),
       arrow::field("braking", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 6), false),
       arrow::field("accel", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 6), false),
       arrow::field("orientation", arrow::boolean(), false),
       arrow::field("small_speed_var", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 13), false),
       arrow::field("large_speed_var", arrow::fixed_size_list(arrow::field("item", arrow::uint64(), false), 13), false),
       arrow::field("accel_decel", arrow::uint64(), false), arrow::field("speed_changes", arrow::uint64(), false)});
  return result;
}

// simdjson DOM style API
inline auto SimdTripParse0(const TripParserWorkload& data) -> TripParserResult {
  TripParserResult result("simdjson", "DOM", false);

  result.timer.Start();
  result.values = std::vector<uint64_t>();
  result.offsets = std::vector<int32_t>();
  result.timer.Split();

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();

  for (auto obj : objects) {
    // Push back offset for this object.
    result.offsets.push_back(static_cast<int32_t>(result.values.size()));
    for (auto elem : obj["voltage"]) {
      result.values.push_back(elem.get_uint64());
    }
  }
  // Push back end offset.
  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();

  result.Finish();
  return result;
}
