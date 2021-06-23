#pragma once

#include <charconv>

enum class ErrorCode { SUCCESS = 0, INVALID_ARGUMENT, VALUE_OUT_OF_RANGE, UNEXPECTED_ARRAY_VALUE_SEPARATOR };

auto ToString(ErrorCode ec) -> std::string {
  switch (ec) {
    case ErrorCode::SUCCESS:
      return "success";
    case ErrorCode::INVALID_ARGUMENT:
      return "invalid argument";
    case ErrorCode::VALUE_OUT_OF_RANGE:
      return "value out of range";
    case ErrorCode::UNEXPECTED_ARRAY_VALUE_SEPARATOR:
      return "unexpected array value separator";
  }
  return "unexpected error code";
}

/**
 * \brief Parses array of primitives from start till end or until end of array.
 * \tparam     T     The type of the primitive.
 * \param[in]  start The start of the JSON bytes.
 * \param[in]  end   The end of the JSON bytes.
 * \param[out] next  A pointer to the next JSON byte to parse.
 * \param[out] out   The output vector to append to.
 * \return An error code indicating SUCCESS or some type of failure.
 */
template <typename T>
inline auto ParseArrayPrim(const char* start, const char* end, const char** next, std::vector<T>* out) -> ErrorCode {
  while (start != end) {
    T val = 0;
    auto fcr = std::from_chars<T>(start, end, val);
    switch (fcr.ec) {
      default:
        break;  // no errors, continue.
      case std::errc::invalid_argument:
        *next = start;
        return ErrorCode::INVALID_ARGUMENT;
      case std::errc::result_out_of_range:
        *next = start;
        return ErrorCode::VALUE_OUT_OF_RANGE;
    }
    out->push_back(val);
    start = fcr.ptr;

    // Check for end of array.
    if (*start == ']') {
      start += 1;
      *next = start;
      return ErrorCode::SUCCESS;
    }

    // Check for invalid value separator.
    if (*start != ',') {
      *next = start;
      return ErrorCode::UNEXPECTED_ARRAY_VALUE_SEPARATOR;
    }

    start++;
  }
  // When there have been no errors until the end, return with success.
  *next = start;
  return ErrorCode::SUCCESS;
}
