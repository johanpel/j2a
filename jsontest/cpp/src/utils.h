#pragma once

#include <cmath>
#include <type_traits>
#include <vector>

enum class Scale { Unit, K, M, G, Mi, Ki, Gi };

auto ScaleMultiplier(Scale scale) -> double {
  switch (scale) {
    default:
      return 1.;
    case Scale::K:
      return 1e3;
    case Scale::M:
      return 1e6;
    case Scale::G:
      return 1e9;
    case Scale::Ki:
      return std::pow(2, 10);
    case Scale::Mi:
      return std::pow(2, 20);
    case Scale::Gi:
      return std::pow(2, 30);
  }
}

template <typename T>
auto DataSizeOf(const T& thing) -> size_t {
  static_assert(std::is_arithmetic<T>());
  return sizeof(T);
}

template <>
auto DataSizeOf(const std::string& thing) -> size_t {
  return thing.size();
}

template <typename T>
auto DataSizeOf(const std::vector<T>& thing) -> size_t {
  if (std::is_arithmetic<T>()) {
    return thing.size() * sizeof(T);
  } else {
    size_t size = 0;
    for (const auto& item : thing) {
      size += DataSizeOf(item);
    }
    return size;
  }
}

template <typename T>
auto ApplyScale(const T& thing, Scale scale = Scale::Unit) -> double {
  return DataSizeOf(thing) / ScaleMultiplier(scale);
}

template <typename T>
auto GetThroughput(const T& thing, double seconds, Scale scale = Scale::Unit) -> double {
  return ApplyScale(thing, scale) / seconds;
}

template <typename T>
auto Sum(const std::vector<T>& vector) -> T {
  static_assert(std::is_arithmetic_v<T>);
  T result = 0;
  for (const auto& val : vector) {
    result += val;
  }
  return result;
}