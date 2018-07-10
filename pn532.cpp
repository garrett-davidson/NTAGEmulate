#include "pn532.h"

#include <libserialport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef linux
#define SP_MODE_READ_WRITE (sp_mode)(SP_MODE_READ | SP_MODE_WRITE)
#endif

#define DEBUGGING

#ifdef DEBUGGING
#define MAX_RESPONSE_TIME 100 // Bigger time out when debugging
#else
#define MAX_RESPONSE_TIME 15 // (ms) defined by PN532 spec
#endif

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

  printf("\nFrame:\n");
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
  case RxInListPassiveTarget: {
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

  case RxInDataExchange:
    printf("InDataExchange\n");
    printf("Status: %d\n", frame[RESPONSE_PREFIX_LENGTH + 1]);

    printf("Data: ");
    printHex(frame + RESPONSE_PREFIX_LENGTH + 2, dataLength - 2);
    break;

  case TxTgInitAsTarget:
    printf("TgInitAsTarget\n");
    break;

  case RxTgInitAsTarget:
    printf("TgInitastarget\n");
    break;

  case RxTgGetInitiatorCommand:
    printf("TgGetInitiatorCommand\n");
    break;

  default:
    printf("Unknown frame type: %X\n", frameType);
    break;
  }

  printf("\n");
}

PN532::PN532(const char* portName) {
  if (sp_get_port_by_name(portName, &port)) {
    printf("Could not open port\n");
    exit(1);
  }

  if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
    printf("Could not open port\n");
    exit(1);
  }

  if (sp_set_baudrate(port, 115200) != SP_OK) {
    printf("Could not set baud\n");
    exit(1);
  }

  if (sp_set_bits(port, 8) != SP_OK) {
    printf("Could not set data bit\n");
    exit(1);
  }

  if (sp_set_parity(port, SP_PARITY_NONE) != SP_OK) {
    printf("Could not set parity bit\n");
    exit(1);
  }

  if (sp_set_stopbits(port, 1) != SP_OK) {
    printf("Coulld not set stop bits\n");
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

  printf("\nWoke device\n");

  printf("\nFetching firmware version\n");
  const int responseBufferSize = 13;
  uint8_t responseBuffer[responseBufferSize];
  uint8_t command[1] = { TxGetFirmwareVersion };
  if (sendCommand(command, 1, responseBuffer, responseBufferSize) < 0) {
    printf("Could not get firmware version\n");
    return -1;
  }

  return 0;
}

int PN532::setUp() {
  printf("Setting up\n");
  if (samConfig(SamConfigurationModeNormal) < 0) {
    printf("Error SAM config\n");
    return -1;
  }
  return 0;
}

int PN532::setParameters(uint8_t parameters) {
  printf("Setting parameters\n");
  const int commandSize = 2;
  uint8_t command[commandSize] = { TxSetParameters, parameters };

  const int responseBufferSize = 10;
  uint8_t responseBuffer[responseBufferSize];

  if (sendCommand(command, commandSize, responseBuffer, responseBufferSize) < 0) {
    printf("Error setting parameters\n");
    return -1;
  }

  return !(responseBuffer[RESPONSE_PREFIX_LENGTH] == 0x13);
}

int PN532::sendCommand(const uint8_t *command, int commandSize, uint8_t *responseBuffer, const size_t responseBufferSize, int timeout) {
  // TODO: Fully implement timeout
  // <0 = retry
  //  0 = block indefinitely
  // >0 = timeout value
  int ackResponse = 0;
  int responseSize = 0;
  do {
    printf("Sending command\n");
    if (sendFrame(command, commandSize) < 0) {
      printf("Sending error\n");
      return -1;
    }

    ackResponse = awaitAck();

    if (ackResponse == -2) {
      printf("Did not receive ACK\n");
      continue;
    } else if (ackResponse < 0) {
      printf("Ack error\n");
      return -1;
    }

    do {
      responseSize = getResponse(responseBuffer, responseBufferSize);
    } while (!timeout && !responseSize);

    if (responseSize < 0) {
      printf("Response error\n");
      return -1;
    }
  } while (!ackResponse && !responseSize && timeout < 0);

  if (responseSize > 0) {
    printf("Got response:\n");
    printHex(responseBuffer, responseSize);
  } else {
    printf("No response\n");
  }
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
  printFrame(buffer, totalSize);

  return sp_blocking_write(port, buffer, totalSize, 10000) != totalSize;
}

int PN532::awaitAck() {
  const int bufferSize = 100;
  uint8_t buffer[bufferSize];

  const int bytesToRead = 6; // Full ACK/NACK and the useful part of error message

  int responseSize = sp_blocking_read(port, buffer, bytesToRead, MAX_RESPONSE_TIME);

  if (responseSize == 0) {
    printf("Timed out waiting for ACK\n");
    return -2;
  }

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
    return -4;
  }

  printf("Unknown response:\n");
  printHex(buffer, responseSize);
  return -3;
}

int PN532::getResponse(uint8_t *responseBuffer, int responseBufferSize) {
  size_t size = sp_blocking_read(port, responseBuffer, responseBufferSize, MAX_RESPONSE_TIME);
  return size;
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
  printf("Configuring SAM\n");
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

int PN532::ntag2xxReadPage(uint8_t page, uint8_t *buffer) {
  const int pageSize = 16;

  const int commandSize = 4;
  uint8_t command[commandSize] = {
    TxInDataExchange,
    1, // Selected tag
    NTAG21xReadPage,
    page
  };

  const int responseBufferSize = 100; // Max returned data is 262
  uint8_t responseBuffer[responseBufferSize];
  int responseSize = sendCommand(command, commandSize, responseBuffer, responseBufferSize, MAX_RESPONSE_TIME);

  if (responseSize < 0) {
    printf("Error reading page: %d\n", responseSize);
    return -1;
  }

  printf("Read page:\n");
  printFrame(responseBuffer, responseSize);

  memcpy(buffer, responseBuffer + RESPONSE_PREFIX_LENGTH + 2, pageSize);

  return 0;
}

int PN532::initAsTarget(uint8_t mode, const uint8_t *mifareParams, uint8_t responseBuffer[], const size_t responseBufferSize) {
  printf("Initializing as target\n");
  uint8_t command[] = {
    TxTgInitAsTarget,
    TargetModePassiveOnly,

    // Mifare Params
    0, 0, // SENS_RES read from tag
    0, 0, 0, // First 3 bytes of UID
    0, // SEL_RES read from tag

    // FeliCa Params
    0, 0, 0, 0, 0, 0, 0, 0, // 8 bytes NFCID2t
    0, 0, 0, 0, 0, 0, 0, 0, // 8 bytes PAD
    0, 0, // 2 bytes System Code

    // NFCID3t
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, // Length of general bytes (max 47)
    // General bytes would go here

    0, // Length of historical bytes (max 48)
    // Historical bytes would go here
  };

  memcpy(command + 2, mifareParams, 6);

  return sendCommand(command, sizeof(command), responseBuffer, responseBufferSize, 0);
}

int PN532::getInitiatorCommand(uint8_t *responseBuffer, const size_t responseBufferSize) {
  const uint8_t command[] = { 0x88 };
  return sendCommand(command, 1, responseBuffer, responseBufferSize);
}

int PN532::writeRegister(uint16_t registerAddress, uint8_t registerValue) {
  const int commandSize = 4;
  const uint8_t command[commandSize] = {
    TxWriteRegister,
    registerAddress >> 8, // High bytes of address
    registerAddress & 0xFF, // Low bytes of address
    registerValue,
  };

  const int responseBufferSize = 100;
  uint8_t responseBuffer[responseBufferSize];
  int responseSize = sendCommand(command, commandSize, responseBuffer, responseBufferSize);

  if (responseBuffer[RESPONSE_PREFIX_LENGTH] != 0x09) {
    printf("Error writing register\n");
    printHex(responseBuffer, responseSize);
    return -1;
  }

  return responseSize;
}

int PN532::ntag2xxEmulate(const uint8_t *uid, const uint8_t *data) {
  // TODO: Investigate what happens with FeliCa emulation

  printf("\nSetting registers\n");
  int responseSize = writeRegister(RegisterCIU_TxMode,
                1 << 7 // TxCRCEn automatically handle CRC
                | 0 // TxSpeed 000 = 106 kbit/s
                | 0 << 3 // InvMod don't invert modulation
                | 0 << 2 // TxMix don't mix SIGIN with internal coder
                | 0 // TxFraming 00 = ISO14443A
                );

  if (responseSize < 0) {
    printf("Error writing to tx register\n");
    return -1;
  }

    responseSize = writeRegister(RegisterCIU_RxMode,
                1 << 7 // TxCRCEn automatically handle CRC
                | 0 // TxSpeed 000 = 106 kbit/s
                | 1 << 3 // RxNoErr ignore invalid streams
                | 0 << 2 //  RxMultiple  receive multiple frames at once
                | 0 // RxFraming 00 = ISO14443A
                );


  if (responseSize < 0) {
    printf("Error writing to tx register\n");
    return -1;
  }

  printf("\nEmulating tag\n");
  const uint8_t mifareParams[] = {
    0x44, 0x00, // SENS_RES read from tag
    uid[0], uid[1], uid[2], // First 3 bytes of UID
    0x00, // SEL_RES read from tag
  };

  const int responseBufferSize = 300; // Initiator command can be up to 262
  uint8_t responseBuffer[responseBufferSize];

  responseSize = initAsTarget(TargetModePassiveOnly, mifareParams, responseBuffer, responseBufferSize);

  printf("Got init:\n");
  printFrame(responseBuffer, responseSize);

  printf("Changing settings\n");
  responseSize = writeRegister(RegisterCIU_RxMode, 0); // Disbable Rx CRC
  if (responseSize < 0) {
    printf("Error writing to rx register\n");
    return -1;
  }

  responseSize = writeRegister(RegisterCIU_TxMode, 0); // Disable Tx CRC
  if (responseSize < 0) {
    printf("Error writing to rx register\n");
    return -1;
  }

  responseSize = writeRegister(RegisterCIU_ManualRCV, 1 << 3); // Disable Parity

  printf("Initialized\n");
  while (responseSize > 0) {
    uint8_t status = responseBuffer[RESPONSE_PREFIX_LENGTH + 1];

    if (status == 0x02) {
      printf("CRC Error\n");
      //writeRegister(uint16_t registerAddress, uint8_t registerValue)

    } else {
      printf("Status OK\n");
    }

    uint8_t responseCommand = responseBuffer[RESPONSE_PREFIX_LENGTH + 2];

    uint8_t *nextCommand;
    int nextCommandSize;

    switch (responseCommand) {
    case NTAG21xReadPage: {
      const int commandSize = 17;
      uint8_t command[commandSize] = { TxTgResponseToInitiator, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      uint8_t page = responseBuffer[RESPONSE_PREFIX_LENGTH + 3];
      printf("Sending page: %X\n", page);
      memcpy(command + 1, data + (page * 4), 16);

      nextCommand = command;
      nextCommandSize = commandSize;
      break;
    }

    case 0xA0:
    case 0x79:
    case NTAG21xHalt:
      nextCommandSize = 0;
      printf("Halting\n");
      //sleep(3);
      break;
      // return 0;

    default:
      printf("Received unknown command: %X\n", responseCommand);
      return -1;
    }

    if (nextCommandSize)
      responseSize = sendCommand(nextCommand, nextCommandSize, responseBuffer, responseBufferSize);

    if (responseSize < 0) {
      printf("Error sending response\n");
      return -2;
    }

    printf("Getting next command\n");
    responseSize = getInitiatorCommand(responseBuffer, responseBufferSize);
  }

  return 0;
}
