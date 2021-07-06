#pragma once

#include <arrow/api.h>
#include <putong/timer.h>
#include <rapidjson/rapidjson.h>
#include <simdjson.h>
#include <spdlog/spdlog.h>

#include <boost/spirit/home/x3.hpp>
#include <charconv>
#include <iostream>
#include <numeric>
#include <utility>

auto schema_trip() -> std::shared_ptr<arrow::Schema> {
  static auto result = arrow::schema(
      {arrow::field("timestamp", arrow::utf8(), false),       //
       arrow::field("timezone", arrow::uint64(), false),      //
       arrow::field("vin", arrow::uint64(), false),           //
       arrow::field("odometer", arrow::uint64(), false),      //
       arrow::field("hypermiling", arrow::boolean(), false),  //
       arrow::field("avgspeed", arrow::uint64(), false),      //
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
       arrow::field("accel_decel", arrow::uint64(), false),  //
       arrow::field("speed_changes", arrow::uint64(), false)});
  return result;
}

struct TripParserWorkload {
  size_t num_jsons = 1;
  size_t num_bytes = 0;
  std::vector<char> bytes;

  void Finish() {
    num_bytes = bytes.size();
    bytes = std::vector<char>();
  }
};

struct TripBuilder {
  TripBuilder()
      : timestamp(std::make_shared<arrow::StringBuilder>()),
        timezone(std::make_shared<arrow::UInt64Builder>()),
        vin(std::make_shared<arrow::UInt64Builder>()),
        odometer(std::make_shared<arrow::UInt64Builder>()),
        hypermiling(std::make_shared<arrow::BooleanBuilder>()),
        avgspeed(std::make_shared<arrow::UInt64Builder>()),
        sec_in_band(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                  std::make_shared<arrow::UInt64Builder>(), 12)),
        miles_in_time_range(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                          std::make_shared<arrow::UInt64Builder>(), 24)),
        const_speed_miles_in_band(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                                std::make_shared<arrow::UInt64Builder>(), 12)),
        vary_speed_miles_in_band(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                               std::make_shared<arrow::UInt64Builder>(), 12)),
        sec_decel(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                std::make_shared<arrow::UInt64Builder>(), 10)),
        sec_accel(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                std::make_shared<arrow::UInt64Builder>(), 10)),
        braking(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                              std::make_shared<arrow::UInt64Builder>(), 6)),
        accel(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                            std::make_shared<arrow::UInt64Builder>(), 6)),
        orientation(std::make_shared<arrow::BooleanBuilder>()),
        small_speed_var(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                      std::make_shared<arrow::UInt64Builder>(), 13)),
        large_speed_var(std::make_shared<arrow::FixedSizeListBuilder>(arrow::default_memory_pool(),
                                                                      std::make_shared<arrow::UInt64Builder>(), 13)),
        accel_decel(std::make_shared<arrow::UInt64Builder>()),
        speed_changes(std::make_shared<arrow::UInt64Builder>()) {}

  auto Finish() -> std::shared_ptr<arrow::RecordBatch> {
    std::vector<std::shared_ptr<arrow::Array>> arrays = {timestamp->Finish().ValueOrDie(),
                                                         timezone->Finish().ValueOrDie(),
                                                         vin->Finish().ValueOrDie(),
                                                         odometer->Finish().ValueOrDie(),
                                                         hypermiling->Finish().ValueOrDie(),
                                                         avgspeed->Finish().ValueOrDie(),
                                                         sec_in_band->Finish().ValueOrDie(),
                                                         miles_in_time_range->Finish().ValueOrDie(),
                                                         const_speed_miles_in_band->Finish().ValueOrDie(),
                                                         vary_speed_miles_in_band->Finish().ValueOrDie(),
                                                         sec_decel->Finish().ValueOrDie(),
                                                         sec_accel->Finish().ValueOrDie(),
                                                         braking->Finish().ValueOrDie(),
                                                         accel->Finish().ValueOrDie(),
                                                         orientation->Finish().ValueOrDie(),
                                                         small_speed_var->Finish().ValueOrDie(),
                                                         large_speed_var->Finish().ValueOrDie(),
                                                         accel_decel->Finish().ValueOrDie(),
                                                         speed_changes->Finish().ValueOrDie()};
    
    auto result = arrow::RecordBatch::Make(schema_trip(), arrays[0]->length(), arrays);
    assert(result != nullptr);

    return result;
  }

  std::shared_ptr<arrow::StringBuilder> timestamp;
  std::shared_ptr<arrow::UInt64Builder> timezone;
  std::shared_ptr<arrow::UInt64Builder> vin;
  std::shared_ptr<arrow::UInt64Builder> odometer;
  std::shared_ptr<arrow::BooleanBuilder> hypermiling;
  std::shared_ptr<arrow::UInt64Builder> avgspeed;
  std::shared_ptr<arrow::FixedSizeListBuilder> sec_in_band;
  std::shared_ptr<arrow::FixedSizeListBuilder> miles_in_time_range;
  std::shared_ptr<arrow::FixedSizeListBuilder> const_speed_miles_in_band;
  std::shared_ptr<arrow::FixedSizeListBuilder> vary_speed_miles_in_band;
  std::shared_ptr<arrow::FixedSizeListBuilder> sec_decel;
  std::shared_ptr<arrow::FixedSizeListBuilder> sec_accel;
  std::shared_ptr<arrow::FixedSizeListBuilder> braking;
  std::shared_ptr<arrow::FixedSizeListBuilder> accel;
  std::shared_ptr<arrow::BooleanBuilder> orientation;
  std::shared_ptr<arrow::FixedSizeListBuilder> small_speed_var;
  std::shared_ptr<arrow::FixedSizeListBuilder> large_speed_var;
  std::shared_ptr<arrow::UInt64Builder> accel_decel;
  std::shared_ptr<arrow::UInt64Builder> speed_changes;
};

struct TripParserResult {
  TripParserResult(std::string framework, std::string api, bool pre_alloc)
      : framework(std::move(framework)), api(std::move(api)), output_pre_allocated(pre_alloc) {}

  // Impl. properties
  std::string framework = "null";
  std::string api = "null";
  bool output_pre_allocated = false;

  // Builder
  TripBuilder builder;

  // Result batch after finish
  std::shared_ptr<arrow::RecordBatch> batch;

  // Statistics
  putong::SplitTimer<3> timer;
  size_t num_bytes = 0;

  void Print() const {}

  void Finish() { batch = builder.Finish(); }

  [[nodiscard]] bool IsEqual(const TripParserResult& other) const {
    if (!batch->Equals(*other.batch)) {
      std::cerr << "Batch not equal." << std::endl;
    } else if (num_bytes != other.num_bytes) {
      std::cerr << "No. bytes: " << num_bytes << " != " << other.num_bytes << std::endl;
    } else {
      return true;
    }
    return false;
  }
};

// simdjson DOM style API
inline auto SimdTripParse0(const TripParserWorkload& data) -> TripParserResult {
  TripParserResult result("simdjson", "DOM", false);

  result.timer.Start();
  // potential allocations
  result.timer.Split();

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();

  for (auto obj : objects) {
    std::string_view timestamp = obj["timestamp"].get_string().value();
    result.builder.timestamp->Append(arrow::util::string_view(timestamp.data(), timestamp.length()));

    result.builder.timezone->Append(obj["timezone"].get_uint64().value());
    result.builder.vin->Append(obj["vin"].get_uint64().value());
    result.builder.odometer->Append(obj["odometer"].get_uint64().value());
    result.builder.hypermiling->Append(obj["hypermiling"].get_bool().value());
    result.builder.avgspeed->Append(obj["avgspeed"].get_uint64().value());

    result.builder.sec_in_band->Append();
    for (const auto& elem : obj["sec_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.sec_in_band->value_builder())->Append(elem.get_uint64().value());
    }

    result.builder.miles_in_time_range->Append();
    for (const auto& elem : obj["miles_in_time_range"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.miles_in_time_range->value_builder())
          ->Append(elem.get_uint64().value());
    }

    result.builder.const_speed_miles_in_band->Append();
    for (const auto& elem : obj["const_speed_miles_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.const_speed_miles_in_band->value_builder())
          ->Append(elem.get_uint64().value());
    }

    result.builder.vary_speed_miles_in_band->Append();
    for (const auto& elem : obj["vary_speed_miles_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.vary_speed_miles_in_band->value_builder())
          ->Append(elem.get_uint64().value());
    }

    result.builder.sec_decel->Append();
    for (const auto& elem : obj["sec_decel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.sec_decel->value_builder())->Append(elem.get_uint64().value());
    }

    result.builder.sec_accel->Append();
    for (const auto& elem : obj["sec_accel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.sec_accel->value_builder())->Append(elem.get_uint64().value());
    }

    result.builder.braking->Append();
    for (const auto& elem : obj["braking"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.braking->value_builder())->Append(elem.get_uint64().value());
    }

    result.builder.accel->Append();
    for (const auto& elem : obj["accel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.accel->value_builder())->Append(elem.get_uint64().value());
    }

    auto orientation = obj["orientation"].get_bool().value();
    result.builder.orientation->Append(orientation);

    result.builder.small_speed_var->Append();
    for (const auto& elem : obj["small_speed_var"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.small_speed_var->value_builder())->Append(elem.get_uint64().value());
    }

    result.builder.large_speed_var->Append();
    for (const auto& elem : obj["large_speed_var"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(result.builder.large_speed_var->value_builder())->Append(elem.get_uint64().value());
    }

    result.builder.accel_decel->Append(obj["accel_decel"].get_uint64().value());
    result.builder.speed_changes->Append(obj["speed_changes"].get_uint64().value());
  }

  result.timer.Split();

  result.Finish();

  return result;
}
