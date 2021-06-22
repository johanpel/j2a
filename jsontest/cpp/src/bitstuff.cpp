#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

auto ToString(const char chars[64]) -> std::string {
  std::string result(64, ' ');
  for (size_t i = 0; i < 64; i++) {
    result[i] = chars[i];
  }
  return result;
}

auto ToString(uint64_t val) -> std::string {
  std::stringstream ss;
  for (size_t i = 0; i < 64; i++) {
    ss << ((val >> i & 1u) == 1 ? '1' : '.');
  }
  return ss.str();
}

template <size_t PrefixSize = 16>
void hline() {
  std::cout << std::setw(PrefixSize + 3) << "" << std::string(64, '_') << std::endl;
}

template <size_t PrefixSize = 16>
void print(const std::string& prefix, uint64_t value) {
  std::cout << std::setw(PrefixSize) << prefix << " : " << ToString(value) << std::endl;
}

template <size_t PrefixSize = 16>
void print(const std::string& prefix, const std::string& value) {
  assert(value.length() == 64);
  std::cout << std::setw(PrefixSize) << prefix << " : " << ToString(value.data())
            << std::endl;
}

auto ByteCompare(const char chars[64], char cmp) -> uint64_t {
  uint64_t result = 0;
  for (size_t i = 0; i < 64; i++) {
    result |= chars[i] == cmp ? (1ul << i) : 0;
  }
  return result;
}

auto CarrylessMultiply(uint64_t a, uint64_t b) -> uint64_t {
  uint64_t result = 0;
  for (size_t i = 0; i < 64; i++) {
    if (a >> i & 1ul) {
      result = result xor (b << i);
    }
  }
  return result;
}

auto bitstuff() -> int {
  std::string json =
      R"({ "\\\"Nam[{": [ 116,"\\\\" , 234, "true", false ], "t":"\\\"" })";

  constexpr uint64_t even = 0x5555555555555555ul;
  constexpr uint64_t odd = 0xAAAAAAAAAAAAAAAAul;
  uint64_t bitmap = ByteCompare(json.data(), '\\');
  uint64_t starts = bitmap & ~(bitmap << 1u);
  uint64_t even_starts = starts & even;
  uint64_t even_carries = bitmap + even_starts;
  uint64_t even_carries_only = even_carries & ~bitmap;
  uint64_t odd_1 = even_carries_only & ~even;
  uint64_t odd_starts = starts & odd;
  uint64_t odd_carries = bitmap + odd_starts;
  uint64_t odd_carries_only = odd_carries & ~bitmap;
  uint64_t odd_2 = odd_carries_only & ~odd;
  uint64_t odd_sequence_ends = odd_1 | odd_2;

  uint64_t quotes = ByteCompare(json.data(), '"');
  uint64_t quoted_ends = quotes & ~odd_sequence_ends;
  uint64_t quoted_range = CarrylessMultiply(quoted_ends, ~0ul);

  std::cout << std::setw(22) << "LSB" << std::setw(61) << "MSB" << std::endl;
  print("In", json);
  print("B", bitmap);
  print("E", even);
  print("O", odd);
  print("S", starts);
  hline();
  print("ES", even_starts);
  print("EC", even_carries);
  print("ECE", even_carries_only);
  print("OD1", odd_1);
  hline();
  print("OS", odd_starts);
  print("OC", odd_carries);
  print("OCE", odd_carries_only);
  print("OD2", odd_2);
  hline();
  print("OD", odd_sequence_ends);
  hline();
  print("In", json);
  print("Q", quotes);
  print("OD", odd_sequence_ends);
  print("Q &= ~OD", quoted_ends);
  print("CLMUL(Q, ~0)", quoted_range);

  return EXIT_SUCCESS;
}