#pragma once

#include <arrow/api.h>
#include <illex/arrow.h>

#include <vector>

#include "./battery.h"
#include "./trip.h"

auto GenerateBatteryParserWorkload(size_t max_array_values, size_t approx_size, bool as_array = false,
                                   size_t capacity_padding = 0, int seed = 0) -> BatteryParserWorkload {
  BatteryParserWorkload result;

  result.max_array_values = max_array_values;

  illex::GenerateOptions opts;
  opts.seed = seed;
  auto gen = illex::FromArrowSchema(*schema_battery(max_array_values), opts);

  if (as_array) {
    result.bytes.push_back('[');
  }
  while (result.bytes.size() < approx_size) {
    auto str = gen.GetString();
    result.num_jsons++;
    result.bytes.insert(result.bytes.end(), str.begin(), str.end());
    if (as_array && (result.bytes.size() + 2 >= approx_size)) {
      result.bytes.push_back(',');
    } else {
      result.bytes.push_back('\n');
    }
  }
  if (as_array) {
    result.bytes.push_back(']');
  }

  if ((capacity_padding != 0) && (result.bytes.capacity() % capacity_padding != 0)) {
    result.bytes.reserve(result.bytes.capacity() % capacity_padding);
  }

  return result;
}

auto GenerateTripParserWorkload(size_t approx_size, bool as_array = false, size_t capacity_padding = 0, int seed = 0)
    -> TripParserWorkload {
  TripParserWorkload result;

  illex::GenerateOptions opts;
  opts.seed = seed;
  auto gen = illex::FromArrowSchema(*schema_trip(), opts);

  if (as_array) {
    result.bytes.push_back('[');
  }
  while (result.bytes.size() < approx_size) {
    auto str = gen.GetString();
    result.num_jsons++;
    result.bytes.insert(result.bytes.end(), str.begin(), str.end());
    if (as_array && (result.bytes.size() + 2 >= approx_size)) {
      result.bytes.push_back(',');
    } else {
      result.bytes.push_back('\n');
    }
  }
  if (as_array) {
    result.bytes.push_back(']');
  }

  if ((capacity_padding != 0) && (result.bytes.capacity() % capacity_padding != 0)) {
    result.bytes.reserve(result.bytes.capacity() % capacity_padding);
  }

  return result;
}
