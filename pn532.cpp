#include "pn532.h"

#include <libserialport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESPONSE_TIME 15 // (ms) defined by PN532 spec

#define RESPONSE_PREFIX_LENGTH 6

void PN532::printHex(const uint8_t buffer[], int size) {
  for (int i = 0; i < size; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\n");
}

void PN532::printFrame(const uint8_t *frame, const size_t frameLength) {

  if (frameLength < 4) {
    printf("Incomplete frame\n");
    return;
  }

  if (frameLength == 6) { // Probably ACK
    // TODO: Print ACK/NACK
    // return;
  }

  if (frameLength == 8) { // Probably error
    // TODO: Print error
    // return;
  }

  printf("Frame:\n");
  uint8_t dataLength = frame[3] - 1;

  printf("Data length: %d\n", dataLength);
  uint8_t direction = frame[5];
  if (direction == 0xD4) {
    printf("Host -> PN532\n");
  } else if (direction == 0xD5) {
    printf("PN532 -> Host\n");
  } else {
    printf("Unknown direction: %d\n", direction);
  }

  uint8_t frameType = frame[6];
  printf("FrameType: %X\n", frameType);

  switch (frameType) {
  case RxInListPassiveTarget:
    printf("InListPassiveTarget\n");
    uint8_t tagCount = frame[RESPONSE_PREFIX_LENGTH + 1];
    printf("%d tag\n", tagCount);

    printf("Selected tag: %X\n", frame[RESPONSE_PREFIX_LENGTH + 2]);
    printf("SENS_RES: %X %X\n", frame[RESPONSE_PREFIX_LENGTH + 3], frame[RESPONSE_PREFIX_LENGTH + 4]);
    printf("SEL_RES: %X\n", frame[RESPONSE_PREFIX_LENGTH + 5]);

    int idLen = frame[RESPONSE_PREFIX_LENGTH + 6];
    printf("NFC ID Length: %d\n", idLen);
    printf("NFC ID: ");
    printHex(frame + RESPONSE_PREFIX_LENGTH + 7, idLen);
    break;
  }
}

PN532::PN532(const char* portName) {
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
}

int PN532::wakeUp() {
  const int wakeBufferSize = 16;
  uint8_t wakeBuffer[wakeBufferSize] = { 0x55, 0x55, 0x00, 0x00, 0x00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 };

  if ((int)sp_blocking_write(port, wakeBuffer, wakeBufferSize, 10000) != wakeBufferSize) {
    printf("Error waking\n");
    return -1;
  }

  printf("Woke device\n");

  printf("Fetching firmware version\n");
  const int responseBufferSize = 20;
  uint8_t responseBuffer[responseBufferSize];
  uint8_t command[1] = { TxGetFirmwareVersion };
  if (sendCommand(command, 1, responseBuffer, responseBufferSize) < 0) {
    printf("Could not get firmware version\n");
    return -1;
  }

  return 0;
}

int PN532::setUp() {
  if (samConfig(SamConfigurationModeNormal) < 0) {
    printf("Error SAM config\n");
    return -1;
  }
  return 0;
}

int PN532::sendCommand(const uint8_t *command, int commandSize, uint8_t *responseBuffer, const size_t responseBufferSize) {
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

  printf("Got response:\n");
  printHex(responseBuffer, responseSize);
  return responseSize;
}

int PN532::sendFrame(const uint8_t *data, int size) {
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

int PN532::awaitAck() {
  const int bufferSize = 100;
  uint8_t buffer[bufferSize];

  const int bytesToRead = 6; // Full ACK/NACK and the useful part of error message

  int responseSize = sp_blocking_read(port, buffer, bytesToRead, 0);

  if (responseSize != bytesToRead) {
    printf("ACK read error: %d\n", responseSize);
    printHex(buffer, responseSize);
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

int PN532::awaitResponse(uint8_t *responseBuffer, int responseBufferSize) {
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

int PN532::readTagId(uint8_t *idBuffer, uint8_t idBufferLength, uint8_t tagBaudRate) {
  const int commandLength = 3;
  uint8_t command[commandLength] = { TxInListPassiveTarget, 1, tagBaudRate };

  const int responseBufferSize = 100;
  uint8_t responseBuffer[responseBufferSize];

  int responseSize = sendCommand(command, commandLength, responseBuffer, responseBufferSize);
  if (responseSize < 0) {
    printf("Error reading tag id\n");
    return -1;
  }

  printFrame(responseBuffer, responseSize);

  int idLength = responseBuffer[RESPONSE_PREFIX_LENGTH + 6];
  int writeIdLength = idLength < idBufferLength ? idLength : idBufferLength;
  memcpy(idBuffer, responseBuffer + RESPONSE_PREFIX_LENGTH + 7, writeIdLength);

  return writeIdLength;
}

int PN532::samConfig(SamConfigurationMode mode, uint8_t timeout) {
  const int responseBufferSize = 50;
  uint8_t responseBuffer[responseBufferSize];

  const int commandSize = 3;
  uint8_t command[commandSize] = { TxSAMConfiguration, mode, timeout };

  int responseSize = sendCommand(command, commandSize, responseBuffer, responseBufferSize);
  if (responseSize < 0) {
    printf("SAM config error: %d\n", responseSize);
    return -1;
  }

  return responseBuffer[RESPONSE_PREFIX_LENGTH + 0] == RxSAMConfiguration ? 0 : -2;
}
