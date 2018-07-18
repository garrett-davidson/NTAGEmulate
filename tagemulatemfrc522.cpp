#include "tagemulatemfrc522.h"

#include "logger.h"
#include "mfrc522.h"

MFRC522 *device;
int main(int argc, char **argv) {
  device = new MFRC522("/dev/spidev0.0");


}
