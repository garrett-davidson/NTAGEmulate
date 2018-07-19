#include "logger.h"
#include "mfrc522.h"

#include <stdio.h>

MFRC522 *device;
int main(int argc, char **argv) {
  device = new MFRC522("/dev/spidev0.0");
  device->setUp();

  const size_t reqaSizeBits = 7;
  const uint8_t reqa[1] = { 0x26 };

  const size_t receiveBufferSize = 100;
  uint8_t receiveBuffer[receiveBufferSize];
  //device->transceiveBits(reqa, reqaSizeBits, receiveBuffer, receiveBufferSize * 8, 0);
  //printf("Calling test\n");
  device->transceiveBits(reqa, reqaSizeBits, receiveBuffer, receiveBufferSize * 8, 0);
}
