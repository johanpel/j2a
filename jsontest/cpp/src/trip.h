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
  TripBuilder(int64_t pre_alloc_rows = 0, int64_t pre_alloc_ts_values = 0)
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
        speed_changes(std::make_shared<arrow::UInt64Builder>()) {
    timestamp->Reserve(pre_alloc_rows);
    timestamp->ReserveData(pre_alloc_ts_values);
    timezone->Reserve(pre_alloc_rows);
    vin->Reserve(pre_alloc_rows);
    odometer->Reserve(pre_alloc_rows);
    hypermiling->Reserve(pre_alloc_rows);
    avgspeed->Reserve(pre_alloc_rows);
    sec_in_band->value_builder()->Reserve(pre_alloc_rows * 12);
    miles_in_time_range->value_builder()->Reserve(pre_alloc_rows * 24);
    const_speed_miles_in_band->value_builder()->Reserve(pre_alloc_rows * 12);
    vary_speed_miles_in_band->value_builder()->Reserve(pre_alloc_rows * 12);
    sec_decel->value_builder()->Reserve(pre_alloc_rows * 10);
    sec_accel->value_builder()->Reserve(pre_alloc_rows * 10);
    braking->value_builder()->Reserve(pre_alloc_rows * 6);
    accel->value_builder()->Reserve(pre_alloc_rows * 6);
    orientation->Reserve(pre_alloc_rows);
    small_speed_var->value_builder()->Reserve(pre_alloc_rows * 13);
    large_speed_var->value_builder()->Reserve(pre_alloc_rows * 13);
    accel_decel->Reserve(pre_alloc_rows);
    speed_changes->Reserve(pre_alloc_rows);
  }

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

  // Result batch after finish
  std::shared_ptr<arrow::RecordBatch> batch;

  // Statistics
  putong::SplitTimer<3> timer;
  size_t num_bytes = 0;

  void Print() const {}

  void Finish() {}

  [[nodiscard]] bool Equals(const TripParserResult& other) const {
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
  TripBuilder builder;
  result.timer.Split();  // pre allocations

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();  // parse

  for (auto obj : objects) {
    std::string_view timestamp = obj["timestamp"].get_string().value();
    builder.timestamp->Append(arrow::util::string_view(timestamp.data(), timestamp.length()));

    builder.timezone->Append(obj["timezone"].get_uint64().value());
    builder.vin->Append(obj["vin"].get_uint64().value());
    builder.odometer->Append(obj["odometer"].get_uint64().value());
    builder.hypermiling->Append(obj["hypermiling"].get_bool().value());
    builder.avgspeed->Append(obj["avgspeed"].get_uint64().value());

    builder.sec_in_band->Append();
    for (const auto& elem : obj["sec_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.sec_in_band->value_builder())->Append(elem.get_uint64().value());
    }

    builder.miles_in_time_range->Append();
    for (const auto& elem : obj["miles_in_time_range"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.miles_in_time_range->value_builder())->Append(elem.get_uint64().value());
    }

    builder.const_speed_miles_in_band->Append();
    for (const auto& elem : obj["const_speed_miles_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.const_speed_miles_in_band->value_builder())
          ->Append(elem.get_uint64().value());
    }

    builder.vary_speed_miles_in_band->Append();
    for (const auto& elem : obj["vary_speed_miles_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.vary_speed_miles_in_band->value_builder())->Append(elem.get_uint64().value());
    }

    builder.sec_decel->Append();
    for (const auto& elem : obj["sec_decel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.sec_decel->value_builder())->Append(elem.get_uint64().value());
    }

    builder.sec_accel->Append();
    for (const auto& elem : obj["sec_accel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.sec_accel->value_builder())->Append(elem.get_uint64().value());
    }

    builder.braking->Append();
    for (const auto& elem : obj["braking"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.braking->value_builder())->Append(elem.get_uint64().value());
    }

    builder.accel->Append();
    for (const auto& elem : obj["accel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.accel->value_builder())->Append(elem.get_uint64().value());
    }

    auto orientation = obj["orientation"].get_bool().value();
    builder.orientation->Append(orientation);

    builder.small_speed_var->Append();
    for (const auto& elem : obj["small_speed_var"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.small_speed_var->value_builder())->Append(elem.get_uint64().value());
    }

    builder.large_speed_var->Append();
    for (const auto& elem : obj["large_speed_var"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.large_speed_var->value_builder())->Append(elem.get_uint64().value());
    }

    builder.accel_decel->Append(obj["accel_decel"].get_uint64().value());
    builder.speed_changes->Append(obj["speed_changes"].get_uint64().value());
  }

  result.timer.Split();  // walk & convert DOM

  result.batch = builder.Finish();
  result.Finish();

  return result;
}

// simdjson DOM style API
inline auto SimdTripParse1(const TripParserWorkload& data, int64_t pre_alloc_rows, int64_t pre_alloc_ts_values)
    -> TripParserResult {
  TripParserResult result("simdjson", "DOM", true);

  result.timer.Start();
  TripBuilder builder(pre_alloc_rows, pre_alloc_ts_values);
  result.timer.Split();  // pre allocations

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();  // parse

  for (auto obj : objects) {
    std::string_view timestamp = obj["timestamp"].get_string().value();
    builder.timestamp->UnsafeAppend(arrow::util::string_view(timestamp.data(), timestamp.length()));

    builder.timezone->UnsafeAppend(obj["timezone"].get_uint64().value());
    builder.vin->UnsafeAppend(obj["vin"].get_uint64().value());
    builder.odometer->UnsafeAppend(obj["odometer"].get_uint64().value());
    builder.hypermiling->UnsafeAppend(obj["hypermiling"].get_bool().value());
    builder.avgspeed->UnsafeAppend(obj["avgspeed"].get_uint64().value());

    builder.sec_in_band->Append();
    for (const auto& elem : obj["sec_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.sec_in_band->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    builder.miles_in_time_range->Append();
    for (const auto& elem : obj["miles_in_time_range"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.miles_in_time_range->value_builder())
          ->UnsafeAppend(elem.get_uint64().value());
    }

    builder.const_speed_miles_in_band->Append();
    for (const auto& elem : obj["const_speed_miles_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.const_speed_miles_in_band->value_builder())
          ->UnsafeAppend(elem.get_uint64().value());
    }

    builder.vary_speed_miles_in_band->Append();
    for (const auto& elem : obj["vary_speed_miles_in_band"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.vary_speed_miles_in_band->value_builder())
          ->UnsafeAppend(elem.get_uint64().value());
    }

    builder.sec_decel->Append();
    for (const auto& elem : obj["sec_decel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.sec_decel->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    builder.sec_accel->Append();
    for (const auto& elem : obj["sec_accel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.sec_accel->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    builder.braking->Append();
    for (const auto& elem : obj["braking"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.braking->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    builder.accel->Append();
    for (const auto& elem : obj["accel"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.accel->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    auto orientation = obj["orientation"].get_bool().value();
    builder.orientation->UnsafeAppend(orientation);

    builder.small_speed_var->Append();
    for (const auto& elem : obj["small_speed_var"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.small_speed_var->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    builder.large_speed_var->Append();
    for (const auto& elem : obj["large_speed_var"].get_array()) {
      dynamic_cast<arrow::UInt64Builder*>(builder.large_speed_var->value_builder())->UnsafeAppend(elem.get_uint64().value());
    }

    builder.accel_decel->UnsafeAppend(obj["accel_decel"].get_uint64().value());
    builder.speed_changes->UnsafeAppend(obj["speed_changes"].get_uint64().value());
  }

  result.timer.Split();  // walk & convert DOM

  result.batch = builder.Finish();
  result.Finish();

  return result;
}

// custom pre alloc
inline auto STLTripParse0(const TripParserWorkload& data, int64_t pre_alloc_rows, int64_t pre_alloc_ts_values)
    -> TripParserResult {
  TripParserResult result("custom", "null", true);

  result.timer.Start();
  TripBuilder builder(pre_alloc_rows, pre_alloc_ts_values);
  result.timer.Split();  // pre allocations

  const auto* pos = data.bytes.data();
  const auto* end = pos + data.bytes.size();

  // Eat any initial whitespace.
  pos = EatWhitespace(pos, end);

  // Start parsing objects
  while ((pos < end) && (pos != nullptr)) {
    pos = EatObjectStart(pos, end);  // {
    pos = EatWhitespace(pos, end);

    pos = EatMemberKey(pos, end, "timestamp");
    pos = EatWhitespace(pos, end);
    pos = EatMemberKeyValueSeperator(pos, end);
    pos = EatWhitespace(pos, end);
    pos = EatStringWithoutEscapes(pos, end, builder.timestamp.get());
    pos = EatWhitespace(pos, end);
    pos = EatChar(pos, end, ',');

    pos = EatUInt64MemberUnsafe(pos, end, "timezone", builder.timezone.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "vin", builder.vin.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "odometer", builder.odometer.get(), true);
    pos = EatBoolMemberUnsafe(pos, end, "hypermiling", builder.hypermiling.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "avgspeed", builder.avgspeed.get(), true);

    // todo: make macros
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "sec_in_band", builder.sec_in_band.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.sec_in_band->value_builder()),
                                              true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(
        pos, end, "miles_in_time_range", builder.miles_in_time_range.get(),
        reinterpret_cast<arrow::UInt64Builder*>(builder.miles_in_time_range->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(
        pos, end, "const_speed_miles_in_band", builder.const_speed_miles_in_band.get(),
        reinterpret_cast<arrow::UInt64Builder*>(builder.const_speed_miles_in_band->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(
        pos, end, "vary_speed_miles_in_band", builder.vary_speed_miles_in_band.get(),
        reinterpret_cast<arrow::UInt64Builder*>(builder.vary_speed_miles_in_band->value_builder()), true);
    pos =
        EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "sec_decel", builder.sec_decel.get(),
                                            reinterpret_cast<arrow::UInt64Builder*>(builder.sec_decel->value_builder()), true);
    pos =
        EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "sec_accel", builder.sec_accel.get(),
                                            reinterpret_cast<arrow::UInt64Builder*>(builder.sec_accel->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "braking", builder.braking.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.braking->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "accel", builder.accel.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.accel->value_builder()), true);
    pos = EatBoolMemberUnsafe(pos, end, "orientation", builder.orientation.get(), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "small_speed_var", builder.small_speed_var.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.small_speed_var->value_builder()),
                                              true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "large_speed_var", builder.large_speed_var.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.large_speed_var->value_builder()),
                                              true);

    pos = EatUInt64MemberUnsafe(pos, end, "accel_decel", builder.accel_decel.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "speed_changes", builder.speed_changes.get(), false);

    pos = EatWhitespace(pos, end);
    pos = EatObjectEnd(pos, end);  // }
    pos = EatWhitespace(pos, end);
    pos = EatChar(pos, end, '\n');

    // The newline may be the last byte, check if we didn't reach end of input before continuing.
    if ((pos < end) && (pos != nullptr)) {
      pos = EatWhitespace(pos, end);
    }
  }
  result.timer.Split();
  // walk dom
  result.timer.Split();

  result.batch = builder.Finish();
  result.Finish();
  return result;
}

// custom
inline auto STLTripParse1(const TripParserWorkload& data, int64_t pre_alloc_rows, int64_t pre_alloc_ts_values)
    -> TripParserResult {
  TripParserResult result("custom", "null", true);

  result.timer.Start();
  TripBuilder builder(pre_alloc_rows, pre_alloc_ts_values);
  result.timer.Split();  // pre allocations

  const auto* pos = data.bytes.data();
  const auto* end = pos + data.bytes.size();

  // Eat any initial whitespace.
  pos = EatWhitespace(pos, end);

  // Start parsing objects
  while ((pos < end) && (pos != nullptr)) {
    pos = EatObjectStart(pos, end);  // {
    pos = EatWhitespace(pos, end);

    pos = EatMemberKey(pos, end, "timestamp");
    pos = EatWhitespace(pos, end);
    pos = EatMemberKeyValueSeperator(pos, end);
    pos = EatWhitespace(pos, end);
    pos = EatStringWithoutEscapes(pos, end, builder.timestamp.get());
    pos = EatWhitespace(pos, end);
    pos = EatChar(pos, end, ',');

    pos = EatUInt64MemberUnsafe(pos, end, "timezone", builder.timezone.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "vin", builder.vin.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "odometer", builder.odometer.get(), true);
    pos = EatBoolMemberUnsafe(pos, end, "hypermiling", builder.hypermiling.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "avgspeed", builder.avgspeed.get(), true);

    // todo: make macros
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "sec_in_band", builder.sec_in_band.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.sec_in_band->value_builder()),
                                              true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(
        pos, end, "miles_in_time_range", builder.miles_in_time_range.get(),
        reinterpret_cast<arrow::UInt64Builder*>(builder.miles_in_time_range->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(
        pos, end, "const_speed_miles_in_band", builder.const_speed_miles_in_band.get(),
        reinterpret_cast<arrow::UInt64Builder*>(builder.const_speed_miles_in_band->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(
        pos, end, "vary_speed_miles_in_band", builder.vary_speed_miles_in_band.get(),
        reinterpret_cast<arrow::UInt64Builder*>(builder.vary_speed_miles_in_band->value_builder()), true);
    pos =
        EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "sec_decel", builder.sec_decel.get(),
                                            reinterpret_cast<arrow::UInt64Builder*>(builder.sec_decel->value_builder()), true);
    pos =
        EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "sec_accel", builder.sec_accel.get(),
                                            reinterpret_cast<arrow::UInt64Builder*>(builder.sec_accel->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "braking", builder.braking.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.braking->value_builder()), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "accel", builder.accel.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.accel->value_builder()), true);
    pos = EatBoolMemberUnsafe(pos, end, "orientation", builder.orientation.get(), true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "small_speed_var", builder.small_speed_var.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.small_speed_var->value_builder()),
                                              true);
    pos = EatUInt64FixedSizeArrayMemberUnsafe(pos, end, "large_speed_var", builder.large_speed_var.get(),
                                              reinterpret_cast<arrow::UInt64Builder*>(builder.large_speed_var->value_builder()),
                                              true);

    pos = EatUInt64MemberUnsafe(pos, end, "accel_decel", builder.accel_decel.get(), true);
    pos = EatUInt64MemberUnsafe(pos, end, "speed_changes", builder.speed_changes.get(), false);

    pos = EatWhitespace(pos, end);
    pos = EatObjectEnd(pos, end);  // }
    pos = EatWhitespace(pos, end);
    pos = EatChar(pos, end, '\n');

    // The newline may be the last byte, check if we didn't reach end of input before continuing.
    if ((pos < end) && (pos != nullptr)) {
      pos = EatWhitespace(pos, end);
    }
  }
  result.timer.Split();
  // walk dom
  result.timer.Split();

  result.batch = builder.Finish();
  result.Finish();
  return result;
}

class UnsafeRapidTripHandler {
 public:
  explicit UnsafeRapidTripHandler(int64_t pre_alloc_rows = 0, int64_t pre_alloc_ts_values = 0)
      : builder(pre_alloc_rows, pre_alloc_ts_values) {}

  bool Null() {
    std::cerr << "Unexpected null." << std::endl;
    return false;
  }
  bool Bool(bool b) {
    std::cerr << "Unexpected bool." << std::endl;
    return false;
  }
  bool Int(int i) {
    std::cerr << "Unexpected int." << std::endl;
    return false;
  }
  bool Uint(unsigned i) {
    values_bld->UnsafeAppend(static_cast<uint64_t>(i));
    return true;
  }
  bool Int64(int64_t i) {
    std::cerr << "Unexpected int64." << std::endl;
    return false;
  }
  bool Uint64(uint64_t i) {
    values_bld->UnsafeAppend(i);
    return true;
  }
  bool Double(double d) {
    std::cerr << "Unexpected double." << std::endl;
    return false;
  }
  bool RawNumber(const char* str, rapidjson::SizeType length, bool copy) {
    std::cerr << "Unexpected raw number." << std::endl;
    return false;
  }
  bool String(const char* str, rapidjson::SizeType length, bool copy) {
    std::cerr << "Unexpected string." << std::endl;
    return false;
  }
  bool StartObject() { return true; }
  bool Key(const char* str, rapidjson::SizeType length, bool copy) { return true; }
  bool EndObject(rapidjson::SizeType memberCount) { return true; }
  bool StartArray() {
    list_bld->Append();
    return true;
  }
  bool EndArray(rapidjson::SizeType elementCount) { return true; }

  TripBuilder builder;

  arrow::FixedSizeListBuilder* current_list_bld;
  arrow::UInt64Builder* current_uint64_bld;
  arrow::BooleanBuilder
};