#pragma once

#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

namespace bolson {

inline void StartLogger() {
  auto logger = spdlog::stdout_logger_mt("");
  logger->set_pattern("[%n] [%l] %v");
  spdlog::set_default_logger(logger);
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#endif
}

}  // namespace bolson
