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

#include "./generic.h"

auto schema_battery(size_t max_array_size = 16) -> std::shared_ptr<arrow::Schema> {
  return arrow::schema(
      {arrow::field("voltage",
                    arrow::list(arrow::field("item", arrow::uint64(), false)
                                    ->WithMetadata(arrow::key_value_metadata({"illex_MIN", "illex_MAX"}, {"0", "2047"}))),
                    false)
           ->WithMetadata(arrow::key_value_metadata({"illex_MIN_LENGTH", "illex_MAX_LENGTH"},
                                                    {"1", std::to_string(max_array_size + 1)}))});
}

struct BatteryParserWorkload {
  size_t max_array_values = 1;
  size_t num_jsons = 1;
  size_t num_bytes = 0;
  std::vector<char> bytes;

  void Finish() {
    num_bytes = bytes.size();
    bytes = std::vector<char>();
  }
};

struct BatteryParserResult {
  BatteryParserResult(std::string framework, std::string api, bool pre_alloc, size_t max_array_values)
      : framework(std::move(framework)), api(std::move(api)), output_pre_allocated(pre_alloc) {
    auto values_bld = std::make_shared<arrow::UInt64Builder>();
    voltage_bld = std::make_shared<arrow::ListBuilder>(arrow::default_memory_pool(), values_bld);
  }

  // Impl. properties
  std::string framework = "null";
  std::string api = "null";
  bool output_pre_allocated = false;

  // Arrow builders
  std::shared_ptr<arrow::ListBuilder> voltage_bld;

  // Arrow output
  std::shared_ptr<arrow::RecordBatch> batch;

  // Statistics
  putong::SplitTimer<3> timer;
  size_t num_bytes_out = 0;
  size_t num_values = 0;
  size_t num_offsets = 0;

  void Finish() {
    num_values = voltage_bld->value_builder()->length();
    num_offsets = voltage_bld->length() + 1;
    num_bytes_out = num_values * sizeof(uint64_t) + num_offsets * sizeof(int32_t);

    std::vector<std::shared_ptr<arrow::Array>> arrays = {voltage_bld->Finish().ValueOrDie()};
    auto result = arrow::RecordBatch::Make(schema_battery(), arrays[0]->length(), arrays);
    assert(result != nullptr);
    batch = result;
  }

  [[nodiscard]] bool Equals(const BatteryParserResult& other) const {
    if (!batch->Equals(*other.batch)) {
      std::cerr << batch->ToString() << std::endl;
      std::cerr << other.batch->ToString() << std::endl;
    }
    return true;
  }
};

size_t get_battery_max_array_size(const arrow::Schema& schema) {
  auto max_val = schema.field(0)->metadata()->Get("illex_MAX_LENGTH");
  return std::strtoul(max_val.ValueOrDie().c_str(), nullptr, 10) - 1;
}

// simdjson DOM style API
inline auto SimdBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("simdjson", "DOM", false, data.max_array_values);

  auto list_bld = result.voltage_bld;
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  // allocations
  result.timer.Split();

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();

  for (auto obj : objects) {
    // Start new list
    result.voltage_bld->Append();
    for (auto elem : obj["voltage"]) {
      values_bld->Append(elem.get_uint64());
    }
  }
  // Last offset
  result.voltage_bld->Append();
  result.timer.Split();

  result.Finish();
  return result;
}

// simdjson DOM style API pre allocated destination
inline auto SimdBatteryParse1(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("simdjson", "DOM", true, data.max_array_values);

  auto list_bld = result.voltage_bld;
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  list_bld->Reserve(alloc_o);
  values_bld->Reserve(alloc_v);
  result.timer.Split();

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();

  size_t values_idx = 0;
  size_t offsets_idx = 0;

  for (auto obj : objects) {
    list_bld->Append();
    for (auto elem : obj["voltage"]) {
      assert(values_idx < alloc_v);
      values_bld->UnsafeAppend(elem.get_uint64());
    }
  }
  // last offset
  list_bld->Append();
  result.timer.Split();

  result.Finish();
  return result;
}

// simdjson dom style API pre allocated destination, not using keys
inline auto SimdBatteryParse2(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("simdjson", "DOM (no keys)", true, data.max_array_values);
  auto list_bld = result.voltage_bld;
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  list_bld->Reserve(alloc_o);
  values_bld->Reserve(alloc_v);
  result.timer.Split();

  simdjson::dom::parser parser;
  auto objects = parser.parse_many(data.bytes.data(), data.bytes.size(), data.bytes.capacity());
  if (objects.error()) {
    std::cerr << simdjson::error_message(objects.error()) << std::endl;
  }
  result.timer.Split();

  size_t values_idx = 0;
  size_t offsets_idx = 0;
  for (auto elem : objects) {
    list_bld->Append();
    offsets_idx++;
    auto obj = elem.get_object().value_unsafe();
    auto arr = obj.begin().value().get_array();
    for (auto e : arr) {
      values_bld->UnsafeAppend(e.get_uint64());
      values_idx++;
    }
  }
  // last offset
  list_bld->Append();
  result.timer.Split();

  result.Finish();
  return result;
}

// rapidjson DOM style API
inline auto RapidBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "DOM", false, data.max_array_values);
  auto list_bld = result.voltage_bld;
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());
  result.timer.Start();
  // allocations
  result.timer.Split();

  rapidjson::StringStream stream(data.bytes.data());
  rapidjson::Document doc;

  while (stream.Tell() != data.bytes.size() - 1) {
    // doc.ParseInsitu<rapidjson::kParseStopWhenDoneFlag>(const_cast<char*>(data.bytes.data()));
    doc.ParseStream<rapidjson::kParseStopWhenDoneFlag>(stream);
    if (doc.HasParseError()) {
      std::cout << "RapidJSON error: " << doc.GetParseError() << std::endl;
      break;
    }
    auto array = doc.GetObject()["voltage"].GetArray();
    list_bld->Append();
    for (auto& e : array) {
      values_bld->Append(e.GetInt64());
    }
  }
  list_bld->Append();

  result.timer.Split();
  // walking dom
  result.timer.Split();

  result.Finish();
  return result;
}

// rapidjson DOM style API, in situ
inline auto RapidBatteryParse1(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "DOM (in situ)", false, data.max_array_values);
  auto list_bld = result.voltage_bld;
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  // allocations
  result.timer.Split();

  rapidjson::InsituStringStream stream(const_cast<char*>(data.bytes.data()));
  rapidjson::Document doc;

  while (stream.Tell() != data.bytes.size() - 1) {
    // doc.ParseInsitu<rapidjson::kParseStopWhenDoneFlag>(const_cast<char*>(data.bytes.data()));
    doc.ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseInsituFlag>(stream);
    if (doc.HasParseError()) {
      std::cout << "RapidJSON error: " << doc.GetParseError() << std::endl;
      break;
    }
    auto array = doc.GetObject()["voltage"].GetArray();
    list_bld->Append();
    for (const auto& e : array) {
      values_bld->Append(e.GetInt64());
    }
  }
  list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

class RapidBatteryHandler {
 public:
  explicit RapidBatteryHandler(arrow::ListBuilder* list_builder, int64_t v_alloc = 0, int64_t o_alloc = 0)
      : list_bld(list_builder), values_bld(dynamic_cast<arrow::UInt64Builder*>(list_builder->value_builder())) {
    list_bld->Reserve(o_alloc);
    values_bld->Reserve(v_alloc);
  }

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
    values_bld->Append(static_cast<uint64_t>(i));
    return true;
  }
  bool Int64(int64_t i) {
    std::cerr << "Unexpected int64." << std::endl;
    return false;
  }
  bool Uint64(uint64_t i) {
    values_bld->Append(i);
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

  arrow::ListBuilder* list_bld;
  arrow::UInt64Builder* values_bld;
};

// rapidjson sax api
inline auto RapidBatteryParse2(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "SAX", false, data.max_array_values);
  result.timer.Start();
  RapidBatteryHandler handler(result.voltage_bld.get());
  result.timer.Split();

  rapidjson::InsituStringStream stream(const_cast<char*>(data.bytes.data()));
  rapidjson::Reader reader;

  while (stream.Tell() != data.bytes.size() - 1) {
    // doc.ParseInsitu<rapidjson::kParseStopWhenDoneFlag>(const_cast<char*>(data.bytes.data()));
    auto parse_result = reader.Parse<rapidjson::kParseStopWhenDoneFlag>(stream, handler);
    if (parse_result.IsError()) {
      std::cout << "RapidJSON error at: " << stream.Tell() << " ec:" << parse_result.Code() << std::endl;
      break;
    }
  }

  handler.list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

class UnsafeRapidBatteryHandler {
 public:
  explicit UnsafeRapidBatteryHandler(arrow::ListBuilder* list_builder, int64_t v_alloc = 0, int64_t o_alloc = 0)
      : list_bld(list_builder), values_bld(dynamic_cast<arrow::UInt64Builder*>(list_builder->value_builder())) {
    list_bld->Reserve(o_alloc);
    values_bld->Reserve(v_alloc);
  }

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

  arrow::ListBuilder* list_bld;
  arrow::UInt64Builder* values_bld;
};

// rapidjson sax api pre allocated
inline auto RapidBatteryParse3(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "SAX", true, data.max_array_values);
  result.timer.Start();
  UnsafeRapidBatteryHandler handler(result.voltage_bld.get(), alloc_v, alloc_o);
  result.timer.Split();

  rapidjson::InsituStringStream stream(const_cast<char*>(data.bytes.data()));
  rapidjson::Reader reader;

  while (stream.Tell() != data.bytes.size() - 1) {
    // doc.ParseInsitu<rapidjson::kParseStopWhenDoneFlag>(const_cast<char*>(data.bytes.data()));
    auto parse_result = reader.Parse<rapidjson::kParseStopWhenDoneFlag>(stream, handler);
    if (parse_result.IsError()) {
      std::cout << "RapidJSON error at: " << stream.Tell() << " ec:" << parse_result.Code() << std::endl;
      break;
    }
  }

  handler.list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

// assume no unnecessary whitespaces anywhere, e.g. minified jsons
// assume ndjson
// assume output size known.
auto STLParseBattery0(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("Custom", "null", true, data.max_array_values);
  auto list_bld = result.voltage_bld;
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  list_bld->Reserve(alloc_o);
  values_bld->Reserve(alloc_v);
  result.timer.Split();

  const char* battery_header = "{\"voltage\":[";
  const size_t max_uint64_len = std::to_string(std::numeric_limits<uint64_t>::max()).length();

  const char* pos = data.bytes.data();
  const char* end = pos + data.bytes.size();

  size_t index = 0;
  size_t offset = 0;

  while (pos < end) {
    // Check header.
    if (std::memcmp(pos, battery_header, strlen(battery_header)) != 0) {
      throw std::runtime_error("Battery header did not correspond to expected header.");
    }
    // Place offset and increase index.
    list_bld->Append();

    pos += strlen(battery_header);
    // Get values.
    while (true) {
      uint64_t val = 0;
      auto val_result = std::from_chars<uint64_t>(pos, pos + max_uint64_len, val);
      switch (val_result.ec) {
        default:
          break;
        case std::errc::invalid_argument:
          throw std::runtime_error(std::string("Battery voltage values contained invalid value: ") +
                                   std::string(pos, max_uint64_len));
        case std::errc::result_out_of_range:
          throw std::runtime_error("Battery voltage value out of uint64_t range.");
      }
      values_bld->UnsafeAppend(val);
      pos = val_result.ptr;

      // Check for end of array.
      if (*pos == ']') {
        pos += 3;  // for "}]\n"
        break;
      }
      if (*pos != ',') {
        throw std::runtime_error("Battery voltage array expected ',' value separator.");
      }
      pos++;
    }
  }

  // Last offset.
  list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

// assume ndjson
// assume output size known
auto STLParseBattery1(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("Custom", "whitespaces", false, data.max_array_values);
  auto list_bld = result.voltage_bld.get();
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  list_bld->Reserve(alloc_o);
  values_bld->Reserve(alloc_v);
  result.timer.Split();

  const auto* pos = data.bytes.data();
  const auto* end = pos + data.bytes.size();

  // Eat any initial whitespace.
  pos = EatWhitespace(pos, end);

  // Start parsing objects
  while ((pos < end) && (pos != nullptr)) {
    pos = EatObjectStart(pos, end);  // {
    pos = EatWhitespace(pos, end);
    pos = EatMemberKey(pos, end, "voltage");  // "voltage"
    pos = EatWhitespace(pos, end);
    pos = EatMemberKeyValueSeperator(pos, end);  // :
    pos = EatWhitespace(pos, end);
    pos = EatUInt64Array(pos, end, list_bld, values_bld);  // e.g. [1,2,3]
    pos = EatWhitespace(pos, end);
    pos = EatObjectEnd(pos, end);  // }
    pos = EatWhitespace(pos, end);
    pos = EatChar(pos, end, '\n');

    // The newline may be the last byte, check if we didn't reach end of input before continuing.
    if ((pos < end) && (pos != nullptr)) {
      pos = EatWhitespace(pos, end);
    }
  }

  // Push last offset.
  list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

struct append_value {
  template <typename Context>
  inline void operator()(Context const& ctx, arrow::UInt64Builder* values_bld) const {
    using namespace boost::spirit;
    values_bld->Append(x3::_attr(ctx));
  }
};

struct append_value_unsafe {
  template <typename Context>
  inline void operator()(Context const& ctx, arrow::UInt64Builder* values_bld) const {
    using namespace boost::spirit;
    values_bld->UnsafeAppend(x3::_attr(ctx));
  }
};

struct append_offset {
  template <typename Context>
  inline void operator()(Context const& ctx, arrow::ListBuilder* list_bld) const {
    using namespace boost::spirit;
    list_bld->Append();
  }
};

template <typename Iterator>
auto parse_minified_battery(Iterator first, Iterator last, arrow::ListBuilder* list_bld, arrow::UInt64Builder* values_bld)
    -> bool {
  using namespace boost::spirit;

  size_t offset = 0;
  auto push_value = std::bind(append_value(), std::placeholders::_1, values_bld);
  auto push_offset = std::bind(append_offset(), std::placeholders::_1, list_bld);

  auto header = x3::lit("{\"voltage\":[")[push_offset];
  auto array = x3::uint64[push_value] >> *(x3::char_(',') >> x3::uint64[push_value]);  // uint64's separated by ,
  auto footer = x3::lit("]}\n");
  auto object = header >> array >> footer;
  auto grammar = *object >> x3::eoi;  // objects separated by newline

  bool result = x3::parse(first, last, grammar);

  if (first != last) {  // fail if we did not get a full match
    std::cerr << "Spirit did not reach end of input... Remaining: ";
    int i = 0;
    while ((first != last) && (i < 64)) {
      std::cerr << *first++;
      i++;
    }
    return false;
  }

  return result;
}

auto SpiritBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("Boost Spirit.X3", "minified", false, data.max_array_values);
  auto list_bld = result.voltage_bld.get();
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  result.timer.Split();

  if (!parse_minified_battery(data.bytes.begin(), data.bytes.end(), list_bld, values_bld)) {
    throw std::runtime_error("Spirit parsing error.");
  }

  list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

template <typename Iterator>
auto parse_battery_prealloced(Iterator first, Iterator last, arrow::ListBuilder* list_bld, arrow::UInt64Builder* values_bld)
    -> bool {
  using namespace boost::spirit;

  size_t offset = 0;
  auto push_value = std::bind(append_value_unsafe(), std::placeholders::_1, values_bld);
  auto push_offset = std::bind(append_offset(), std::placeholders::_1, list_bld);

  auto skip = x3::char_(' ') | x3::char_('\t');
  auto header = x3::char_('{') >> x3::lit("\"voltage\"") >> x3::char_(':') >> x3::char_('[')[push_offset];
  auto array = x3::uint64[push_value] >> *(x3::char_(',') >> x3::uint64[push_value]);  // uint64's separated by ,
  auto footer = x3::char_(']') >> x3::char_('}') >> x3::char_('\n');
  auto object = header >> array >> footer;
  auto grammar = *object >> x3::eoi;  // objects separated by newline

  bool result = x3::phrase_parse(first, last, grammar, skip);

  if (first != last) {  // fail if we did not get a full match
    std::cerr << "Spirit did not reach end of input... Remaining: " << std::endl;
    int i = 0;
    while ((first != last) && (i < 64)) {
      std::cerr << *first++;
      i++;
    }
    if (i == 64) {
      std::cerr << "(" << last - first << " more ...)" << std::endl;
    }
    return false;
  }

  return result;
}

auto SpiritBatteryParse1(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("Boost Spirit.X3", "whitespace", false, data.max_array_values);
  auto list_bld = result.voltage_bld.get();
  auto values_bld = dynamic_cast<arrow::UInt64Builder*>(result.voltage_bld->value_builder());

  result.timer.Start();
  list_bld->Reserve(alloc_o);
  values_bld->Reserve(alloc_v);
  result.timer.Split();

  if (!parse_battery_prealloced(data.bytes.begin(), data.bytes.end(), list_bld, values_bld)) {
    throw std::runtime_error("Spirit parsing error.");
  }

  list_bld->Append();

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}