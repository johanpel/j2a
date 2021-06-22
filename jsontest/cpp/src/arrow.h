#pragma once

#include <arrow/api.h>
#include <illex/arrow.h>

#include <vector>

#include "./battery.h"

auto GenerateBatteryParserWorkload(const arrow::Schema& schema, size_t num_jsons = 1,
                                   bool terminate = false, bool as_array = false,
                                   size_t capacity_padding = 0, int seed = 0)
    -> BatteryParserWorkload {
  BatteryParserWorkload result;

  result.max_array_size = get_battery_max_array_size(schema);
  result.num_jsons = num_jsons;

  illex::GenerateOptions opts;
  opts.seed = seed;
  auto gen = illex::FromArrowSchema(schema, opts);

  if (as_array) {
    result.bytes.push_back('[');
  }
  for (size_t i = 0; i < num_jsons; i++) {
    auto str = gen.GetString();
    result.bytes.insert(result.bytes.end(), str.begin(), str.end());
    if (as_array && (i != num_jsons - 1)) {
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
