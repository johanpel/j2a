#pragma once

#include <arrow/api.h>

inline auto EatWhitespace(const char* pos, const char* end) -> const char* {
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

inline auto EatChar(const char* pos, const char* end, const char c) -> const char* {
  if (*pos != c) {
    throw std::runtime_error(fmt::format("Expected '{}', encountered '{}'", c, *pos));
  }
  pos++;
  if (pos >= end) {
    return nullptr;
  }
  return pos;
}

inline auto EatObjectStart(const char* pos, const char* end) { return EatChar(pos, end, '{'); }
inline auto EatObjectEnd(const char* pos, const char* end) { return EatChar(pos, end, '}'); }
inline auto EatMemberKeyValueSeperator(const char* pos, const char* end) { return EatChar(pos, end, ':'); }
inline auto EatArrayStart(const char* pos, const char* end) { return EatChar(pos, end, '['); }
inline auto EatArrayEnd(const char* pos, const char* end) { return EatChar(pos, end, ']'); }

inline auto EatMemberKey(const char* pos, const char* end, const char* key) -> const char* {
  const size_t key_len = std::strlen(key);
  pos = EatChar(pos, end, '"');
  if (std::memcmp(pos, key, key_len) != 0) {
    throw std::runtime_error(fmt::format("Expected \"{}\", encountered {}", key, std::string_view(pos, key_len)));
  }
  pos += key_len;
  pos = EatChar(pos, end, '"');
  if (pos >= end) {
    return nullptr;
  }
  return pos;
}

// todo: figure out how to use NumericBuilder<T> and from_chars<T> together with the same T so this can be a template function
inline auto EatUInt64(const char* pos, const char* end, arrow::UInt64Builder* builder) -> const char* {
  uint64_t val;
  auto fc_result = std::from_chars<uint64_t>(pos, end, val);
  switch (fc_result.ec) {
    default:
      break;
    case std::errc::invalid_argument:
      throw std::runtime_error(std::string("Cannot parse value as primitive: ") + std::string(pos, end));
    case std::errc::result_out_of_range:
      throw std::runtime_error("Value out of range:" + std::string(pos, end));
  }
  builder->Append(val);
  return fc_result.ptr;
}

inline auto EatUInt64Array(const char* pos, const char* end, arrow::ListBuilder* list_builder,
                           arrow::UInt64Builder* values_builder) -> const char* {
  pos = EatArrayStart(pos, end);  // [
  list_builder->Append();
  // Scan values
  while (true) {
    pos = EatWhitespace(pos, end);
    if (pos > end) {
      throw std::runtime_error("Unexpected end of JSON data while parsing array values..");
    } else if (*pos == ']') {  // Check array end
      pos++;
      break;
    } else {  // Parse values
      uint64_t val = 0;
      pos = EatUInt64(pos, end, values_builder);
      if (*pos == ',') {
        pos++;
      }
    }
  }
  if (pos >= end) {
    return nullptr;
  }
  return pos;
}