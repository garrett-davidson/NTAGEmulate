#include "pn532.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <libserialport.h>

const char *portName = "/dev/tty.usbserial";

struct sp_port *port;

const uint8_t commandGetFirmwareVersion[] = { 0x02 };

inline uint8_t checksum(int x) {
  return 256 - (x + 1);
}

uint8_t sum(uint8_t *data, int size) {
  uint8_t sum = 0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }

  return sum;
}

void serial() {
  const int responseBufferSize = 100;
  uint8_t responseBuffer[responseBufferSize];
  //int readSize = sendCommand(commandGetFirmwareVersion, sizeof(commandGetFirmwareVersion), responseBuffer, responseBufferSize);

  //printHex(responseBuffer, readSize);
}

int main(int argc, char **argv) {
  printf("Initializing NFC adapter\n");

  serial();
  return 0;
}
