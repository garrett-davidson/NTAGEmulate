#include "pn532.h"

#include <libserialport.h>
#include <stdio.h>

const char *portName = "/dev/tty.usbserial";

int main(int argc, char **argv) {
  printf("Initializing NFC adapter\n");

  PN532 *device = new PN532(portName);

  if (device->wakeUp() < 0) { return -1; };
  if (device->setUp() < 0) { return -1; };

  const int idLength = 7;
  uint8_t idBuffer[idLength];
  printf("Fetching tag id\n");
  int receivedIdLength = device->readTagId(idBuffer, idLength, PN532::TypeABaudRate);

  printf("Received id: ");
  device->printHex(idBuffer, receivedIdLength);

  // TODO: Something in nfc-poll is important for SAMConfig to work
  const int pageSize = 16;
  uint8_t pageBuffer[pageSize];
  device->ntag2xxReadPage(1, pageBuffer);
  device->printHex(pageBuffer, 16);

  return 0;
}
