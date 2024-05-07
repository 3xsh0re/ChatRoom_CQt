#include "logger.h"

namespace mtd {

Logger& Logger::Instance() {
  static Logger logger;
  return logger;
}

void Logger::SetLogLevel(LogLevel level) { log_level_ = level; }

void Logger::Log(std::string msg) {
  switch (log_level_) {
    case LogLevel::kInfo:
      std::cout << "[INFO]";
      break;
    case LogLevel::kDebug:
      std::cout << "[DEBUG]";
      break;
    case LogLevel::kError:
      std::cout << "[ERROR]";
      break;
    case LogLevel::kFatal:
      std::cout << "[FATAL]";
      break;
    default:
      break;
  }
  std::cout << "time: " << Timestamp::Now().ToString() << " " << msg << '\n';
}

}  // namespace mtd
