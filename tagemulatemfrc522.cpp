#include "tagemulatemfrc522.h"

#include "logger.h"
#include "mfrc522.h"

#include <stdio.h>

MFRC522 *device;
int main(int argc, char **argv) {
  device = new MFRC522("/dev/spidev0.0");
  device->setUp();

  device->writeRegister(MFRC522::RegisterCommandReg, 1<<3); // Receive

  uint8_t x = 0x14;
  while (x == 0x14) {
    device->readRegister(MFRC522::RegisterCommIrqReg, &x);
  }

  printf("%02X\n", x);
}
