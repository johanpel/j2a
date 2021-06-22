#pragma once

#include <spdlog/spdlog.h>

enum class Status {
  OK = 0,  // Everything is fine.
  Error,   ///< Generic error.
  Arrow,   ///< Arrow error.
};

#define ARROW_ROE(x)                     \
  {                                      \
    auto status__ = x;                   \
    if (!status__.ok()) {                \
      spdlog::error(status__.message()); \
      return Status::Arrow;              \
    }                                    \
  }
