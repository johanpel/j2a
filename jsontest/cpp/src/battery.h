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

  void Finish() {
    checksum = std::accumulate(values.begin(), values.end(), 0UL) + std::accumulate(offsets.begin(), offsets.end(), 0UL);
    num_values = values.size();
    num_offsets = offsets.size();
    num_bytes = values.size() * sizeof(uint64_t);
    values = std::vector<uint64_t>();
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
    result.offsets[offsets_idx] = static_cast<int32_t>(values_idx);
    offsets_idx++;
    for (auto elem : obj["voltage"]) {
      assert(values_idx < alloc_v);
      result.values[values_idx] = elem.get_uint64();
      values_idx++;
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
  RapidBatteryHandler(size_t pre_alloc = 0) : result(std::vector<uint64_t>(pre_alloc)) {}

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
    result.push_back(i);
    return true;
  }
  bool Int64(int64_t i) {
    std::cerr << "Unexpected int64." << std::endl;
    return false;
  }
  bool Uint64(uint64_t i) {
    result.push_back(i);
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

  result.timer.Split();
  result.timer.Split();

  result.values = handler.result;

  result.Finish();
  return result;
}

class FixedSizeBatteryHandler {
 public:
  FixedSizeBatteryHandler(size_t pre_alloc = 0) : result(std::vector<uint64_t>(pre_alloc)) {}

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
    result[index] = i;
    i++;
    return true;
  }
  bool Int64(int64_t i) {
    std::cerr << "Unexpected int64." << std::endl;
    return false;
  }
  bool Uint64(uint64_t i) {
    result[index] = i;
    i++;
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
  bool StartArray() { return true; }
  bool EndArray(rapidjson::SizeType elementCount) { return true; }

  size_t index = 0;
  std::vector<uint64_t> result;
};

// rapidjson sax api pre allocated
inline auto RapidBatteryParse3(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("RapidJSON", "SAX", true);
  result.timer.Start();
  FixedSizeBatteryHandler handler(alloc_v);
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

  result.timer.Split();
  result.timer.Split();
  result.values = handler.result;

  result.Finish();
  return result;
}

// assume no unnecessary whitespaces anywhere, e.g. minified jsons
// assume ndjson
// assume output size known.
auto STLParseBattery0(const BatteryParserWorkload& data, size_t alloc_v, size_t alloc_o) -> BatteryParserResult {
  BatteryParserResult result("Custom", "null", true);

  result.timer.Start();
  result.offsets = std::vector<uint32_t>(alloc_o);
  result.values = std::vector<uint64_t>(alloc_v);
  const char* battery_header = "{\"voltage\":[";
  const auto max_uint64_len = std::to_string(std::numeric_limits<uint64_t>::max()).length();

  const char* pos = data.bytes.data();
  const char* end = pos + data.bytes.size();
  size_t i = 0;
  result.timer.Split();

  while (pos < end) {
    // Check header.
    if (std::memcmp(pos, battery_header, strlen(battery_header)) != 0) {
      throw std::runtime_error("Battery header did not correspond to expected header.");
    }
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
      result.values[i++] = val;
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
  result.offsets = std::vector<uint32_t>();
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

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

// the idea here was to do some error checking etc,..
auto STLParseBattery2(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("Custom", "null", false);
  result.timer.Start();
  result.values = std::vector<uint64_t>();
  const char* battery_header = "{\"voltage\":[";

  const auto* pos = data.bytes.data();
  const auto* end = pos + data.bytes.size();
  result.timer.Split();

  while (pos < end) {
    // Check header.
    if (std::memcmp(pos, battery_header, strlen(battery_header)) != 0) {
      throw std::runtime_error("Battery header did not correspond to expected header.");
    }
    pos += strlen(battery_header);
    // Get values.
    if (ParseArrayPrim<uint64_t>(pos, end, &pos, &result.values) != ErrorCode::SUCCESS) {
      throw std::runtime_error("Could not parse value.");
    }
    pos += 2;  // "}\n";
  }
  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

auto ANTLRBatteryParse0(const BatteryParserWorkload& data) -> BatteryParserResult {
  BatteryParserResult result("ANTLR4", "null", false);
  result.timer.Start();
  result.values = std::vector<uint64_t>();
  result.timer.Split();

  antlr4::ANTLRInputStream input(data.bytes.data(), data.bytes.size());
  BatteryLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  BatteryParser parser(&tokens);

  antlr4::tree::ParseTree* tree = parser.battery();

  // std::cout << "Children: " << tree->children.size() << std::endl;

  // iterate over all except eof
  for (size_t i = 0; i < tree->children.size() - 1; i++) {
    const auto& object = tree->children[i];
    // std::cout << object->toStringTree(&parser) << std::endl;
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

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}

struct append_vector {
  template <typename Context>
  void operator()(Context const& ctx, std::vector<uint64_t>* dest) const {
    using namespace boost::spirit;
    //    std::cout << x3::_attr(ctx) << std::endl;
    dest->push_back(x3::_attr(ctx));
  }
};

template <typename Iterator>
auto parse_battery(Iterator first, Iterator last, std::vector<uint64_t>* dest) -> bool {
  using namespace boost::spirit;
  auto push_back = std::bind(append_vector(), std::placeholders::_1, dest);

  auto header = x3::lit("{\"voltage\":[");
  auto array = x3::uint64[push_back] >> *(x3::char_(',') >> x3::uint64[push_back]);  // uint64's separated by ,
  auto footer = x3::lit("]}\n");
  auto object = header >> array >> footer;
  auto grammar = *object >> x3::eoi;  // objects separated by newline

  bool result = x3::phrase_parse(first, last, grammar, x3::space);

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
  BatteryParserResult result("Boost Spirit.X3", "null", false);
  result.timer.Start();
  result.values = std::vector<uint64_t>();
  result.timer.Split();

  if (!parse_battery(data.bytes.begin(), data.bytes.end(), &result.values)) {
    throw std::runtime_error("Spirit parsing error.");
  }

  result.timer.Split();
  result.timer.Split();

  result.Finish();
  return result;
}
