#ifndef LOGGER_H
#define LOGGER_H
#include <iostream>

#include "noncopyable.h"
#include "timestamp.h"
namespace mtd {
enum LogLevel {
  kInfo,
  kError,
  kFatal,
  kDebug,
};

class Logger : noncopyable {
 public:
  static Logger& Instance();

  void SetLogLevel(LogLevel level);

  void Log(std::string msg);

 private:
  Logger() {}
  int log_level_;
};

#define LOG_ORIGIN(level, logmsg_format, ...)            \
  do {                                                   \
    mtd::Logger& logger = mtd::Logger::Instance();       \
    logger.SetLogLevel(level);                           \
    char buf[1024]{};                                    \
    ::snprintf(buf, 1024, logmsg_format, ##__VA_ARGS__); \
    logger.Log(buf);                                     \
    if (level == mtd::LogLevel::kFatal) {                \
      ::exit(-1);                                        \
    }                                                    \
  } while (0)

#define LOG_INFO(logmsg_format, ...) \
  LOG_ORIGIN(mtd::LogLevel::kInfo, logmsg_format, ##__VA_ARGS__)
#define LOG_ERROR(logmsg_format, ...) \
  LOG_ORIGIN(mtd::LogLevel::kError, logmsg_format, ##__VA_ARGS__)
#define LOG_FATAL(logmsg_format, ...) \
  LOG_ORIGIN(mtd::LogLevel::kFatal, logmsg_format, ##__VA_ARGS__)

#ifdef M_DEBUG
#define LOG_DEBUG(logmsg_format, ...) \
  LOG_ORIGIN(mtd::LogLevel::kDebug, logmsg_format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(logmsg_format, ...)
#endif
}  // namespace mtd

#endif