#include "pn532speed.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void stop_listening(int)
{
  shouldQuit = true;
  printf("Exiting\n");
}

void watchFIFO(uint8_t fifoLevel, int printInterval) {
  uint8_t responseBuffer[500];

  uint8_t fifoData[500];
  int readFIFODataLength = 0;

  uint8_t currentFIFOLevel = 0;
  uint8_t fetchFIFOBuffer[140] = {
    0x00,                       // Preamble
    0x00,
    0xFF,                       // Start of frame
    0x00,                       // Length
    0x00,                       // Length checksum
    0xD4,                       // Direction
    0x06,                       // ReadRegister
  };
  memset(fetchFIFOBuffer + 7, 0, sizeof(fetchFIFOBuffer) - 7);

  writeRegister(RegisterCIU_WaterLevel, fifoLevel);

  const uint8_t fifoPattern[4] = {
    HIGH(RegisterCIU_FIFOData),
    LOW(RegisterCIU_FIFOData),
    HIGH(RegisterCIU_FIFOData),
    LOW(RegisterCIU_FIFOData)
  };
  while (!shouldQuit) {
    awaitInterrupts(InterruptHiAlertIrq);

    currentFIFOLevel = readRegister(RegisterCIU_FIFOLevel);

    // Set length
    // 2 bytes per register read + 1 for Direction + 1 for ReadRegister
    fetchFIFOBuffer[3] = (currentFIFOLevel * 2) + 2;

    // Set length checksum
    fetchFIFOBuffer[4] = (uint8_t)(0 - fetchFIFOBuffer[3]);

    // Set data checksum
    int dcsPosition = 7 + (2*currentFIFOLevel);
    fetchFIFOBuffer[dcsPosition] = (uint8_t)(0
                                             - fetchFIFOBuffer[5]
                                             - fetchFIFOBuffer[6]
                                             - (currentFIFOLevel * HIGH(RegisterCIU_FIFOData))
                                             - (currentFIFOLevel * LOW(RegisterCIU_FIFOData))
                                             );

    // Set postamble
    fetchFIFOBuffer[dcsPosition + 1] = 0;

    // Write RegisterCIU_FIFOData, currentFIFOLevel times
    memset_pattern4(fetchFIFOBuffer + 7, fifoPattern, currentFIFOLevel * 2);

    int fetchFIFOFrameLength = dcsPosition + 2;
    int responseSize = currentFIFOLevel + 9;
    sendFrame(fetchFIFOBuffer, fetchFIFOFrameLength, responseBuffer, responseSize);
    memcpy(fifoData + readFIFODataLength, responseBuffer + 7, currentFIFOLevel);
    readFIFODataLength += currentFIFOLevel;

    if (readFIFODataLength > 100) {
      printf("FIFO Data: %d\n", readFIFODataLength);
      printHex(fifoData, readFIFODataLength);
      readFIFODataLength = 0;
    }
  }
}

int main(int argc, char **argv) {
  signal(SIGINT, stop_listening);

  if (argc != 2) {
    printf("Specify port\n");
    return -1;
  }
  printf("Opening\n");
  openSerialPort(argv[1]);
  printf("Opened\n");

  wakeUp();
  printf("Woke\n");

  pn532SetUp();
  printf("Finished setup\n");

  writeRegister(RegisterCIU_TxMode, 0); // Turn off everything in TxMode
  writeRegister(RegisterCIU_RxMode, 1 << 3 | 1 << 2); // Turn off everything except RxNoErr and RxMultiple

  writeRegister(RegisterCIU_Control, 0 << 4); // This is the import one. Initiator = 0

  const uint8_t receiveCommand = 1 << 3;
  writeRegister(RegisterCIU_Command, receiveCommand);
  while (!shouldQuit) {
    watchFIFO(50, 100);
  }
}
