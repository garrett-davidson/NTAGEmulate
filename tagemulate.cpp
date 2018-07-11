#include "pn532.h"

#include <signal.h>
#include <stdio.h>
#include <libserialport.h>

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

  signal(SIGINT, signalHandler);

  device = new PN532(argv[1]);

  if (device->wakeUp()) return -1;
  if (device->setUp()) return -1;
  if (device->setParameters(PN532::fAutomaticRATS | PN532::fAutomaticATR_RES)) return -1;

  const uint8_t uid[] = { 0x04, 0x17, 0xcd, 0x6a, 0xc5, 0x58, 0x81 };
  device->ntag2xxEmulate(uid, data);

  printf("Finished emulating\n");

  return 0;
}
