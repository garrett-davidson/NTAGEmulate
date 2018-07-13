#ifndef LOGGER_H
#define LOGGER_H
#include <cstdarg>

extern int LogLevel;

enum LogChannel {
  LogChannelSerial = 1 << 0,
};

void log(LogChannel channel, const char *format...);
#endif
