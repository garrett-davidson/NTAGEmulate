#include "pn532.h"

#include <stdio.h>
#include <libserialport.h>

const char *portName = "/dev/tty.usbserial";

int main(int argc, char **argv) {
  printf("Initializing NFC adapter\n");

  PN532 *device = new PN532(portName);

  const uint8_t uid[] = { 0x04, 0x17, 0xcd, 0x6a, 0xc5, 0x58, 0x81 };
  device->ntag2xxEmulate(uid, NULL);

  return 0;
}
