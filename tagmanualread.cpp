#include "pn532.h"
#include "iso14443a-utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

PN532 *device;

bool shouldQuit = false;
void signalHandler(int signal) {
  printf("Signal: %d\n", signal);
  shouldQuit = true;
  device->close();
}

int main(int argc, char **argv) {
  printf("Initializing NFC adapter\n");

  if (argc != 2) {
    printf("Please specify port name\n");
    return -1;
  }

  signal(SIGABRT, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGBUS, signalHandler);
  signal(SIGHUP, signalHandler);
  signal(SIGQUIT, signalHandler);
  signal(SIGTERM, signalHandler);

  device = new PN532(argv[1]);

  if (device->wakeUp()) return -1;
  if (device->setUp(PN532::InitiatorMode)) return -1;

  uint8_t txMode = device->readRegister(PN532::RegisterCIU_TxMode);
  txMode &= 0b01111111; // Disable CRC
  device->writeRegister(PN532::RegisterCIU_TxMode, txMode);

  uint8_t rxMode = device->readRegister(PN532::RegisterCIU_RxMode);
  rxMode &= 0b01111111; // Disable CRC
  device->writeRegister(PN532::RegisterCIU_RxMode, rxMode);

  const int responseFrameSize = 300;
  uint8_t responseFrame[responseFrameSize];
  const uint8_t reqa = 0x26;

  device->writeRegister(0x6305, 0x40); // AutoRFOff
  device->writeRegister(0x633C, 0x10); // Initiator
  device->setParameters(
                        PN532::fAutomaticATR_RES // Enable auto atr_res
                        // Disable automatic RATS
                        );

  printf("Finished setup\n");

  device->sendRawBitsInitiator(&reqa, 7, responseFrame, responseFrameSize);

  uint8_t *responseData = responseFrame + 8;
  int responseDataLength = responseFrame[3] - 3;
  printf("Response: ");
  device->printHex(responseData, responseDataLength);

  if (*responseData != 0x44) {
    printf("Failed REQA\n");
    return -1;
  }

  printf("Got ATQA\n");

  printf("Sending SDD_REQ CL1\n");
  const int sddReqCl1Size = 2;
  const uint8_t sddReqCl1[sddReqCl1Size] = { 0x93, 0x20 };
  device->sendRawBytesInitiator(sddReqCl1, sddReqCl1Size, responseFrame, responseFrameSize);

  const uint8_t cascadeTag = 0x88;
  if (responseData[0] == cascadeTag) {
    printf("Found Cascade level 2 tag\n");
  } else {
    printf("Got unknown result\n");
    return -1;
  }

  const int uidSize = 7;
  uint8_t uid[uidSize] = { responseData[1], responseData[2], responseData[3] };
  uint8_t bcc[2] = { responseData[4] };

  printf("Sending SEL_REQ CL1\n");
  const int selReqCL1Size = 9;
  uint8_t selReqCL1[selReqCL1Size] = { 0x93, 0x70 };
  memcpy(selReqCL1 + 2, responseData, 5);
  *(uint16_t *)(selReqCL1 + selReqCL1Size - 2) = iso14443a_crc(selReqCL1, selReqCL1Size - 2);
  device->sendRawBytesInitiator(selReqCL1, selReqCL1Size, responseFrame, responseFrameSize);

  if (responseData[0] == 0x04) {
    printf("Got SAK for incomplete UID\n");
  } else {
    printf("Got unknown result\n");
    return -1;
  }

  printf("Sending SDD_REQ CL2\n");
  const int sddReqCL2Size = 2;
  const uint8_t sddReqCL2[sddReqCL2Size] = { 0x95, 0x20 };

  int responseSize = device->sendRawBytesInitiator(sddReqCL2, sddReqCL2Size, responseFrame, responseFrameSize);

  if (responseSize == 15) {
    printf("Got the rest of UID\n");
  } else {
    printf("Got unknown result\n");
    return -1;
  }
  memcpy(uid + 3, responseData, 4);
  bcc[1] = responseData[4];

  printf("UID: \n");
  device->printHex(uid, 7);
  printf("BCC: \n");
  device->printHex(bcc, 2);

  delete device;
}
