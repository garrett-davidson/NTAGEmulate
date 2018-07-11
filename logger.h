#include <cstdarg>

extern int LogLevel;

enum LogChannel {
  LogChannelSerial = 1 << 0,
};

void log(LogChannel channel, const char *format...);
