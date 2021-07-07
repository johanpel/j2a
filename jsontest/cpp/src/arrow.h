#pragma once

#include <arrow/api.h>
#include <illex/arrow.h>

#include <vector>

#include "./battery.h"
#include "./trip.h"

auto GenerateBatteryParserWorkload(const arrow::Schema& schema, size_t approx_size, bool as_array = false,
                                   size_t capacity_padding = 0, int seed = 0) -> BatteryParserWorkload {
  BatteryParserWorkload result;

  result.max_array_size = get_battery_max_array_size(schema);

  illex::GenerateOptions opts;
  opts.seed = seed;
  auto gen = illex::FromArrowSchema(schema, opts);

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

// auto GenerateTripParserWorkload(const arrow::Schema& schema, size_t approx_size, bool as_array = false,
//                                    size_t capacity_padding = 0, int seed = 0) -> TripParserWorkload {
//   TripParserWorkload result;

//   illex::GenerateOptions opts;
//   opts.seed = seed;
//   auto gen = illex::FromArrowSchema(schema, opts);

//   if (as_array) {
//     result.bytes.push_back('[');
//   }
//   while (result.bytes.size() < approx_size) {
//     auto str = gen.GetString();
//     result.num_jsons++;
//     result.bytes.insert(result.bytes.end(), str.begin(), str.end());
//     if (as_array && (result.bytes.size() + 2 >= approx_size)) {
//       result.bytes.push_back(',');
//     } else {
//       result.bytes.push_back('\n');
//     }
//   }
//   if (as_array) {
//     result.bytes.push_back(']');
//   }

//   if ((capacity_padding != 0) && (result.bytes.capacity() % capacity_padding != 0)) {
//     result.bytes.reserve(result.bytes.capacity() % capacity_padding);
//   }

//   return result;
// }
