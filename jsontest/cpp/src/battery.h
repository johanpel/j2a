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
#include "BatteryLexer.h"
#include "BatteryParser.h"
#include "antlr4-runtime.h"

struct BatteryParserWorkload {
  size_t max_array_size = 1;
  size_t num_jsons = 1;
  size_t num_bytes = 0;
  std::vector<char> bytes;

  void Finish() {
    num_bytes = bytes.size();
    bytes = std::vector<char>();
  }
};

struct BatteryParserResult {
  BatteryParserResult(std::string framework, std::string api, bool pre_alloc)
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
    auto values_array = std::make_shared<arrow::UInt64Array>(arrow::uint64(), values.size(), arrow::Buffer::Wrap(values));
    auto voltage_column = arrow::ListArray(arrow::list(arrow::uint64()), static_cast<int64_t>(offsets.size() - 1),
                                           arrow::Buffer::Wrap(offsets), values_array);
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

  [[nodiscard]] bool IsEqual(const BatteryParserResult& other) const {
    if (checksum != other.checksum) {
      std::cerr << "Checksum: " << checksum << " != " << other.checksum << std::endl;
    } else if (num_values != other.num_values) {
      std::cerr << "No. values: " << num_values << " != " << other.num_values << std::endl;
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

auto schema_battery(size_t max_array_size = 16) -> std::shared_ptr<arrow::Schema> {
  return arrow::schema(
      {arrow::field("voltage",
                    arrow::list(arrow::field("item", arrow::uint64(), false)
                                    ->WithMetadata(arrow::key_value_metadata({"illex_MIN", "illex_MAX"}, {"0", "2047"}))),
                    false)
           ->WithMetadata(arrow::key_value_metadata({"illex_MIN_LENGTH", "illex_MAX_LENGTH"},
                                                    {"1", std::to_string(max_array_size + 1)}))});
}

size_t get_battery_max_array_size(const arrow::Schema& schema) {
  auto max_val = schema.field(0)->metadata()->Get("illex_MAX_LENGTH");
  return std::strtoul(max_val.ValueOrDie().c_str(), nullptr, 10) - 1;
}

// simdjson DOM style API
inline auto SimdBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("simdjson", "DOM", false);

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

// simdjson DOM style API pre allocated destination
inline auto SimdBatteryParse1(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("simdjson", "DOM", true);

  result.timer.Start();
  result.offsets = std::vector<int32_t>(alloc_o);
  result.values = std::vector<uint64_t>(alloc_v);
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
    result.offsets[offsets_idx++] = static_cast<int32_t>(values_idx);
    for (auto elem : obj["voltage"]) {
      assert(values_idx < alloc_v);
      result.values[values_idx++] = elem.get_uint64();
    }
  }
  // last offset
  result.offsets[offsets_idx] = static_cast<int32_t>(values_idx);

  result.timer.Split();

  result.Finish();
  return result;
}

// simdjson dom style API pre allocated destination, not using keys
inline auto SimdBatteryParse2(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("simdjson", "DOM (no keys)", true);
  result.timer.Start();
  result.offsets = std::vector<int32_t>(alloc_o);
  result.values = std::vector<uint64_t>(alloc_v);
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
    result.offsets[offsets_idx] = static_cast<int32_t>(values_idx);
    offsets_idx++;
    auto obj = elem.get_object().value_unsafe();
    auto arr = obj.begin().value().get_array();
    for (auto e : arr) {
      result.values[values_idx] = e.get_uint64();
      values_idx++;
    }
  }
  // last offset
  result.offsets[offsets_idx] = static_cast<int32_t>(values_idx);

  result.timer.Split();

  result.Finish();
  return result;
}

// rapidjson DOM style API
inline auto RapidBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "DOM", false);
  result.timer.Start();
  result.offsets = std::vector<int32_t>();
  result.values = std::vector<uint64_t>();
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
    result.offsets.push_back(static_cast<int32_t>(result.values.size()));
    for (auto& e : array) {
      result.values.push_back(e.GetInt64());
    }
  }
  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

// rapidjson DOM style API, in situ
inline auto RapidBatteryParse1(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "DOM (in situ)", false);
  result.timer.Start();
  result.offsets = std::vector<int32_t>();
  result.values = std::vector<uint64_t>();
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
    result.offsets.push_back(static_cast<int32_t>(result.values.size()));
    for (const auto& e : array) {
      result.values.push_back(e.GetInt64());
    }
  }
  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

class RapidBatteryHandler {
 public:
  RapidBatteryHandler(size_t v_alloc = 0, size_t o_alloc = 0)
      : values(std::vector<uint64_t>(v_alloc)), offsets(std::vector<int32_t>(o_alloc)) {}

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
    values.push_back(static_cast<uint64_t>(i));
    return true;
  }
  bool Int64(int64_t i) {
    std::cerr << "Unexpected int64." << std::endl;
    return false;
  }
  bool Uint64(uint64_t i) {
    values.push_back(i);
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
  bool StartObject() {
    offsets.push_back(static_cast<int32_t>(values.size()));
    return true;
  }
  bool Key(const char* str, rapidjson::SizeType length, bool copy) { return true; }
  bool EndObject(rapidjson::SizeType memberCount) { return true; }
  bool StartArray() { return true; }
  bool EndArray(rapidjson::SizeType elementCount) { return true; }

  std::vector<int32_t> offsets;
  std::vector<uint64_t> values;
};

// rapidjson sax api
inline auto RapidBatteryParse2(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "SAX", false);
  result.timer.Start();
  RapidBatteryHandler handler;
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

  handler.offsets.push_back(static_cast<int32_t>(handler.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.values = std::move(handler.values);
  result.offsets = std::move(handler.offsets);

  result.Finish();
  return result;
}

class FixedSizeBatteryHandler {
 public:
  FixedSizeBatteryHandler(size_t v_alloc, size_t o_alloc)
      : values(std::vector<uint64_t>(v_alloc)), offsets(std::vector<int32_t>(o_alloc)) {}

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
    values[offset++] = i;
    return true;
  }
  bool Int64(int64_t i) {
    std::cerr << "Unexpected int64." << std::endl;
    return false;
  }
  bool Uint64(uint64_t i) {
    values[offset++] = i;
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
  bool StartObject() {
    offsets[index++] = static_cast<int32_t>(offset);
    return true;
  }
  bool Key(const char* str, rapidjson::SizeType length, bool copy) { return true; }
  bool EndObject(rapidjson::SizeType memberCount) { return true; }
  bool StartArray() { return true; }
  bool EndArray(rapidjson::SizeType elementCount) { return true; }

  // Index into the offsets buffer
  size_t index = 0;
  // Offset of the values buffer.
  size_t offset = 0;

  std::vector<uint64_t> values;
  std::vector<int32_t> offsets;
};

// rapidjson sax api pre allocated
inline auto RapidBatteryParse3(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "SAX", true);
  result.timer.Start();
  FixedSizeBatteryHandler handler(alloc_v, alloc_o);
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

  // TODO: move this to handler finish function
  handler.offsets[handler.index] = static_cast<int32_t>(handler.offset);

  result.timer.Split();
  result.timer.Split();
  result.values = std::move(handler.values);
  result.offsets = std::move(handler.offsets);

  result.Finish();
  return result;
}

// assume no unnecessary whitespaces anywhere, e.g. minified jsons
// assume ndjson
// assume output size known.
auto STLParseBattery0(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("Custom", "null", true);

  result.timer.Start();
  result.offsets = std::vector<int32_t>(alloc_o);
  result.values = std::vector<uint64_t>(alloc_v);

  const char* battery_header = "{\"voltage\":[";
  const size_t max_uint64_len = std::to_string(std::numeric_limits<uint64_t>::max()).length();

  const char* pos = data.bytes.data();
  const char* end = pos + data.bytes.size();

  size_t index = 0;
  size_t offset = 0;

  result.timer.Split();

  while (pos < end) {
    // Check header.
    if (std::memcmp(pos, battery_header, strlen(battery_header)) != 0) {
      throw std::runtime_error("Battery header did not correspond to expected header.");
    }
    // Place offset and increase index.
    result.offsets[index++] = static_cast<int32_t>(offset);

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
      result.values[offset++] = val;
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
  result.offsets[index++] = static_cast<int32_t>(offset);

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

// assume no unnecessary whitespaces anywhere, e.g. minified jsons
// assume ndjson
auto STLParseBattery1(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("Custom", "null", false);

  result.timer.Start();
  result.values = std::vector<uint64_t>();
  result.offsets = std::vector<int32_t>();
  const char* battery_header = "{\"voltage\":[";
  const auto max_uint64_len = std::to_string(std::numeric_limits<uint64_t>::max()).length();

  const char* pos = data.bytes.data();
  const char* end = pos + data.bytes.size();
  result.timer.Split();

  while (pos < end) {
    // Check header.
    if (std::memcmp(pos, battery_header, strlen(battery_header)) != 0) {
      throw std::runtime_error("Battery header did not correspond to expected header.");
    }
    // Place offset and increase index.
    result.offsets.push_back(static_cast<int32_t>(result.values.size()));

    pos += strlen(battery_header);
    // Get values.
    while (true) {  // fixme
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
      result.values.push_back(val);
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
  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

inline auto SkipWhitespace(const char* pos, const char* end) -> const char* {
  // Whitespace includes: space, line feed, carriage return, character tabulation
  // const char ws[] = {' ', '\t', '\n', '\r'};
  const char* result = pos;
  while (((*result == ' ') || (*result == '\t')) && (result < end)) {
    result++;
  }
  if (result == end) {
    return nullptr;
  }
  return result;
}

// assume ndjson
auto STLParseBattery2(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("Custom", "whitespaces", false);
  result.timer.Start();
  result.values = std::vector<uint64_t>();

  const auto max_uint64_len = std::to_string(std::numeric_limits<uint64_t>::max()).length();
  const auto* pos = data.bytes.data();
  const auto* end = pos + data.bytes.size();
  result.timer.Split();

  while (pos < end) {
    // Scan for object start
    pos = SkipWhitespace(pos, end);
    if (*pos != '{') {
      throw std::runtime_error(fmt::format("Expected '{', encountered '{}'", *pos));
    }
    pos++;

    // Scan for voltage key
    const char* voltage_key = "\"voltage\"";
    const size_t voltage_key_len = std::strlen(voltage_key);
    pos = SkipWhitespace(pos, end);
    if (std::memcmp(pos, voltage_key, voltage_key_len) != 0) {
      throw std::runtime_error(fmt::format("Expected \"voltage\", encountered {}", std::string_view(pos, voltage_key_len)));
    }
    pos += voltage_key_len;

    // Scan for key-value separator
    pos = SkipWhitespace(pos, end);
    if (*pos != ':') {
      throw std::runtime_error(fmt::format("Expected ':', encountered '{}'", *pos));
    }
    pos++;

    // Scan for array start.
    pos = SkipWhitespace(pos, end);
    if (*pos != '[') {
      throw std::runtime_error(fmt::format("Expected '[', encountered '{}'", *pos));
    }
    pos++;

    // Push offset
    result.offsets.push_back(static_cast<int32_t>(result.values.size()));

    // Scan values
    while (true) {
      uint64_t val = 0;
      pos = SkipWhitespace(pos, end);
      if (pos > end) {
        throw std::runtime_error("Unexpected end of JSON data while parsing array values..");
      } else if (*pos == ']') {  // Check array end
        pos++;
        break;
      } else {  // Parse values
        auto val_result = std::from_chars<uint64_t>(pos, std::min(pos + max_uint64_len, end), val);
        switch (val_result.ec) {
          default:
            break;
          case std::errc::invalid_argument:
            throw std::runtime_error(std::string("Battery voltage values contained invalid value: ") +
                                     std::string(pos, max_uint64_len));
          case std::errc::result_out_of_range:
            throw std::runtime_error("Battery voltage value out of uint64_t range.");
        }
        result.values.push_back(val);

        pos = SkipWhitespace(val_result.ptr, end);
        if (*pos == ',') {
          pos++;
        }
      }
    }

    // Scan for object end
    pos = SkipWhitespace(pos, end);
    if (*pos != '}') {
      throw std::runtime_error(fmt::format("Expected '}', encountered '{}'", *pos));
    }
    pos++;

    // Scan for newline delimiter
    pos = SkipWhitespace(pos, end);
    if (*pos != '\n') {
      throw std::runtime_error(fmt::format("Expected '\\n' (0x20), encountered '{}' (0x{})", *pos, static_cast<uint8_t>(*pos)));
    }
    pos++;
  }

  // Push last offset.
  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

auto ANTLRBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("ANTLR4", "null", false);
  result.timer.Start();
  result.values = std::vector<uint64_t>();
  result.offsets = std::vector<int32_t>();
  result.timer.Split();

  antlr4::ANTLRInputStream input(data.bytes.data(), data.bytes.size());
  BatteryLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  BatteryParser parser(&tokens);

  antlr4::tree::ParseTree* tree = parser.battery();

  // iterate over all except eof
  for (size_t i = 0; i < tree->children.size() - 1; i++) {
    const auto& object = tree->children[i];
    // std::cout << object->toStringTree(&parser) << std::endl;
    result.offsets.push_back(static_cast<int32_t>(result.values.size()));
    // Skip footer, iterate over array contents
    for (auto* child : object->children[1]->children) {
      auto text = child->getText();
      if (text != ",") {
        uint64_t value = 0;
        auto val_result = std::from_chars(text.data(), text.data() + text.length(), value);
        switch (val_result.ec) {
          default:
            break;
          case std::errc::invalid_argument:
            throw std::runtime_error(std::string("Battery voltage values contained invalid value: ") + text);
          case std::errc::result_out_of_range:
            throw std::runtime_error("Battery voltage value " + text + " out of uint64_t range.");
        }
        // std::cout << text << " ";
        result.values.push_back(value);
      }
    }
    // std::cout << std::endl;
  }

  // Last offset.
  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

struct append_value {
  template <typename Context>
  inline void operator()(Context const& ctx, std::vector<uint64_t>* values, size_t* offset) const {
    using namespace boost::spirit;
    values->push_back(x3::_attr(ctx));
    *offset += 1;
  }
};

struct append_offset {
  template <typename Context>
  inline void operator()(Context const& ctx, std::vector<int32_t>* offsets, size_t* offset) const {
    using namespace boost::spirit;
    offsets->push_back(static_cast<int32_t>(*offset));
  }
};

template <typename Iterator>
auto parse_minified_battery(Iterator first, Iterator last, std::vector<uint64_t>* values, std::vector<int32_t>* offsets)
    -> bool {
  using namespace boost::spirit;

  size_t offset = 0;
  auto push_value = std::bind(append_value(), std::placeholders::_1, values, &offset);
  auto push_offset = std::bind(append_offset(), std::placeholders::_1, offsets, &offset);

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
  BatteryParserResult result("Boost Spirit.X3", "minified", false);
  result.timer.Start();
  result.offsets = std::vector<int32_t>();
  result.values = std::vector<uint64_t>();
  result.timer.Split();

  if (!parse_minified_battery(data.bytes.begin(), data.bytes.end(), &result.values, &result.offsets)) {
    throw std::runtime_error("Spirit parsing error.");
  }

  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

template <typename Iterator>
auto parse_battery(Iterator first, Iterator last, std::vector<uint64_t>* values, std::vector<int32_t>* offsets) -> bool {
  using namespace boost::spirit;

  size_t offset = 0;
  auto push_value = std::bind(append_value(), std::placeholders::_1, values, &offset);
  auto push_offset = std::bind(append_offset(), std::placeholders::_1, offsets, &offset);

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

auto SpiritBatteryParse1(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("Boost Spirit.X3", "whitespace", false);
  result.timer.Start();
  result.offsets = std::vector<int32_t>();
  result.values = std::vector<uint64_t>();
  result.timer.Split();

  if (!parse_battery(data.bytes.begin(), data.bytes.end(), &result.values, &result.offsets)) {
    throw std::runtime_error("Spirit parsing error.");
  }

  result.offsets.push_back(static_cast<int32_t>(result.values.size()));

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}