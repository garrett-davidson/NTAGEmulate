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

  delete device;
}
