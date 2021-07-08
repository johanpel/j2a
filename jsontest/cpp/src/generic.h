#pragma once

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

template <typename T>
inline auto EatPrimitive(const char* pos, const char* end, T* dest) -> const char* {
  auto fc_result = std::from_chars<T>(pos, end, *dest);
  switch (fc_result.ec) {
    default:
      break;
    case std::errc::invalid_argument:
      throw std::runtime_error(std::string("Cannot parse value as primitive: ") + std::string(pos, end));
    case std::errc::result_out_of_range:
      throw std::runtime_error("Value out of range:" + std::string(pos, end));
  }
  return fc_result.ptr;
}

template <typename T>
inline auto EatPrimitiveArray(const char* pos, const char* end, std::vector<T>* dest, size_t max_prim_len = 12) -> const char* {
  pos = EatArrayStart(pos, end);  // [
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
      pos = EatPrimitive<uint64_t>(pos, end, &val);
      dest->push_back(val);
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