#include "pn532speed.h"

#include <signal.h>
#include <stdio.h>

static void stop_listening(int)
{
  shouldQuit = true;
  printf("Exiting\n");
}

static const uint8_t listenFrame[] = {
  0x00,                         // Preamble
  0x00,                         //
  0xFF,                         // Start of frame
  0x0E,                         // Length
  (uint8_t)(0 - listenFrame[3]), // Length checksum

  0xD4,                       // Direction
  0x06,                       // ReadRegister,

  // Read number of bytes received
  REG(RegisterCIU_FIFOLevel),

  // Read 4 FIFO bytes
  REG(RegisterCIU_FIFOData),
  REG(RegisterCIU_FIFOData),
  REG(RegisterCIU_FIFOData),
  REG(RegisterCIU_FIFOData),

  REG(RegisterCIU_Error),

  (uint8_t)(0
            - listenFrame[5]
            - listenFrame[6]
            - listenFrame[7]
            - listenFrame[8]
            - listenFrame[9]
            - listenFrame[10]
            - listenFrame[11]
            - listenFrame[12]
            - listenFrame[13]
            - listenFrame[14]
            - listenFrame[15]
            - listenFrame[16]
            - listenFrame[17]
            - listenFrame[18]
            ),                  // Data checksum

  0x00,                         // Postamble
};
const int listenFrameResponseSize = 13;

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
  writeRegister(RegisterCIU_RxMode, 1 << 3); // Turn off everything except RxNoErr

  writeRegister(RegisterCIU_Control, 0 << 4); // This is the import one. Initiator = 0

  uint8_t responseBuffer[100];

  const uint8_t receiveCommand = 1 << 3;
  while (!shouldQuit) {
    clearInterrupts();
    printf("Receiving\n");
    writeRegister(RegisterCIU_Command, receiveCommand);
    printf("Wrote receive\n");
    awaitReceive();
    printf("Received\n");

    sendFrame(listenFrame, sizeof(listenFrame), responseBuffer, listenFrameResponseSize);
    printHex(responseBuffer, listenFrameResponseSize);
  }
}
