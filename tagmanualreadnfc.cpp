#include "logger.h"

#include <nfc/nfc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

nfc_context *context;
nfc_device *device;

void error(const char* errorMessage) {
  printf("%s\n", errorMessage);
  nfc_exit(context);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  nfc_init(&context);
  device = nfc_open(context, NULL);

  if (!device) {
    error("Could not open NFC device\n");
  }

  if (nfc_initiator_init(device) < 0) {
    error("Could not initialize device\n");
  }

  if (nfc_device_set_property_bool(device, NP_HANDLE_CRC, false) < 0) {
    error("Could not disable CRC\n");
  }

  if (nfc_device_set_property_bool(device, NP_EASY_FRAMING, false) < 0) {
    error("Could not disable easy framing\n");
  }

  if (nfc_device_set_property_bool(device, NP_AUTO_ISO14443_4, false) < 0) {
    error("Could not disable Auto RATS\n");
  }

  const int responseDataSize = 300;
  uint8_t responseData[responseDataSize];
  int responseSize = 0;
  printf("Opened NFC device: %s\n\n", nfc_device_get_name(device));

  const uint8_t reqa = 0x26;
  do {
    sleep(1);
    responseSize = nfc_initiator_transceive_bits(device, &reqa, 7, NULL, responseData, responseDataSize, NULL);
  } while (responseSize == -20);
  responseSize /= 8; // This response size is in bits
  printf("Response size: %d\n", responseSize);

  printHex(responseData, responseSize);

  if (*responseData != 0x44) {
    error("Failed REQA\n");
  }

  printf("Got ATQA\n");

}
