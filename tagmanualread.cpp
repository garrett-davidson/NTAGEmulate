#include "pn532.h"

#include <stdio.h>
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

  const int sddReqCl1Size = 2;
  const uint8_t sddReqCl1[sddReqCl1Size] = { 0x93, 0x20 };
  device->sendRawBytesInitiator(sddReqCl1, sddReqCl1Size, responseFrame, responseFrameSize);

  delete device;
}
