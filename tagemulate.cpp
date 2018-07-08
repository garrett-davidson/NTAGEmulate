#include "../libnfc/include/nfc/nfc.h"
#include "../libnfc/libnfc/nfc-internal.h"
//#include "../libnfc/utils/nfc-utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <libserialport.h>

#define MAX_RESPONSE_TIME 15 // (ms) defined by PN532 spec

const char *portName = "/dev/tty.usbserial";

struct sp_port *port;

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

void printHex(const uint8_t buffer[], int size) {
  for (int i = 0; i < size; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n");
}

int sendFrame(uint8_t *data, int size) {
  int totalSize = size + 8;
  uint8_t buffer[totalSize];

  buffer[0] = 0x00; // Preamble
  buffer[1] = 0x00; // Start code 0
  buffer[2] = 0xFF; // Start code 1

  if (size <= 255) {
    buffer[3] = size + 1; // Size of data + size of TFI
    buffer[4] = 0 - (size + 1); // Length checksum (LCS)
    buffer[5] = 0xD4; // Frame indicator (TFI). 0xD4 = controller to PN532, 0xD5 = PN532 to controller
    memcpy(buffer + 6, data, size); // Copy over data

    uint8_t dcs = 0x00; // Equivalent to 256
    dcs -= 0xD4;
    for (int i = 0; i < size; i++) { dcs -= data[i]; }

    buffer[size + 6] = dcs;
    buffer[size + 7] = 0x00; // Postamble
  } else {
    // TODO:
    printf("TODO: Extended frames\n");
  }

  printf("Writing:\n");
  printHex(buffer, totalSize);

  return sp_blocking_write(port, buffer, totalSize, 10000) != totalSize;
}

int awaitAck() {
  const int bufferSize = 100;
  uint8_t buffer[bufferSize];

  const int bytesToRead = 6; // Full ACK/NACK and the useful part of error message

  int responseSize = sp_blocking_read(port, buffer, bytesToRead, MAX_RESPONSE_TIME);

  if (responseSize != bytesToRead) {
    printf("ACK read error\n");
    return -1;
  }

  switch (buffer[3]) {
  case 0:
    if (buffer[4] == 0xFF) { // ACK
      printf("ACK\n");
      return 0;
    }
    break;

  case 0xFF:
    if (buffer[4] == 0x00) { // NACK
      printf("NACK\n");
      return -1;
    }
    break;

  default: // Error
    printf("Error:\n");
    printHex(buffer, responseSize);
    return -2;
  }

  printf("Unknown response:\n");
  printHex(buffer, responseSize);
  return -3;
}

int awaitResponse(uint8_t *responseBuffer, int responseBufferSize) {
  int readSize = 0;

  while (readSize < responseBufferSize) {
    size_t size = sp_blocking_read(port, responseBuffer + readSize, responseBufferSize - readSize, MAX_RESPONSE_TIME);
    if (size > 0) {
      readSize += size;

      if (readSize >= 4) { // Read enough to have a size
        const int frameOverheadSize = 7;
        int frameLength = responseBuffer[3];

        if (readSize >= frameLength + frameOverheadSize) {
          return readSize;
        }
      }
    }
    else { /* timeout */ }
  }
  return 0;
}

int sendCommand(uint8_t *command, int commandSize, uint8_t *responseBuffer, const size_t responseBufferSize) {
  if (sendFrame(command, commandSize) < 0) {
    printf("Sending error\n");
    return -1;
  }

  if (awaitAck() < 0) {
    printf("Ack error\n");
    return -1;
  }

  int responseSize = 0;
  if ((responseSize = awaitResponse(responseBuffer, responseBufferSize)) < 0) {
    printf("Response error\n");
    return -1;
  }

  printf("Got response\n");
  return responseSize;
}


void wakeDevice() {
  uint8_t wakeBuffer[16] = { 0x55, 0x55, 0x00, 0x00, 0x00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 };

  if (sp_blocking_write(port, wakeBuffer, 16, 10000) != 16) {
    printf("Error waking\n");
    exit(1);
  }

  printf("Woke device\n");
}

void serial() {
  if (sp_get_port_by_name(portName, &port)) {
    printf("Could not open port\n");
    exit(1);
  }

  if (sp_open(port, SP_MODE_READ_WRITE) < 0) {
    printf("Could not open port\n");
    exit(1);
  }

  if (sp_set_baudrate(port, 115200) < 0) {
    printf("Could not set baud\n");
    exit(1);
  }

  wakeDevice();

  const int responseBufferSize = 100;
  uint8_t responseBuffer[responseBufferSize];
  uint8_t frame2[] = { 0x14, 0x01 };
  int readSize = sendCommand(frame2, 2, responseBuffer, responseBufferSize);

  printHex(responseBuffer, readSize);
}

int main(int argc, char **argv) {
  printf("Initializing NFC adapter\n");

  serial();
  return 0;
}
