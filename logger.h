#ifndef LOGGER_H
#define LOGGER_H

#include <cstdarg>
#include <stdlib.h>

#ifdef linux
#include <stdint.h>
#endif

extern int LogLevel;

enum LogChannel {
  LogChannelSerial = 1 << 0,
};

void log(LogChannel channel, const char *format...);
void printHex(const uint8_t buffer[], int size, LogChannel logChannel = (LogChannel)0);

#endif
