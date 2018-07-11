#include "logger.h"

#include <stdio.h>

int LogLevel = 0;

void log(LogChannel channel, const char *format...) {
  if ((LogLevel & channel) == 0) return;

  switch (channel) {
  case LogChannelSerial:
    printf("Serial:\t");
  }

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}
