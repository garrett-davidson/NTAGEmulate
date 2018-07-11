#include "pn532.h"

#include <libserialport.h>
#include <signal.h>
#include <stdio.h>

PN532 *device;

bool shouldQuit = false;
void signalHandler(int signal) {
  shouldQuit = true;
  device->close();
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Please specify port\n");
    return -1;
  }

  printf("Initializing NFC adapter\n");

  device = new PN532(argv[1]);

  if (device->wakeUp() < 0) { return -1; };
  if (device->setUp() < 0) { return -1; };

  signal(SIGINT, signalHandler);

  while (!shouldQuit) {
    const int idLength = 7;
    uint8_t idBuffer[idLength];
    printf("Fetching tag id\n");
    int receivedIdLength = device->readTagId(idBuffer, idLength, PN532::TypeABaudRate);

    printf("Received id: ");
    device->printHex(idBuffer, receivedIdLength);

    // TODO: Something in nfc-poll is important for SAMConfig to work
    printf("Fetching page\n");
    const int pageSize = 16;
    uint8_t pageBuffer[pageSize];
    device->ntag2xxReadPage(1, pageBuffer);
    device->printHex(pageBuffer, 16);
  }

  delete device;

  return 0;
}
