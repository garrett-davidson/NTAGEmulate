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

void printHex(const uint8_t buffer[], int size, LogChannel logChannel) {
  for (int i = 0; i < size; i++) {
    if (logChannel) {
      log(logChannel, "%02X ", buffer[i]);
    } else {
      printf("%02X ", buffer[i]);
    }
  }

  if (logChannel) {
    log(logChannel, "\n");
  } else {
    printf("\n");
  }
}
