#include "pn532speed.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static void stop_listening(int)
{
  shouldQuit = true;
  printf("Exiting\n");
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
    sleep(1);

    printFullFIFO();
  }
}
