#include "logger.h"

#include <nfc/nfc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  printf("Sending SDD_REQ CL1\n");
  const int sddReqCl1Size = 2;
  const uint8_t sddReqCl1[sddReqCl1Size] = { 0x93, 0x20 };
  nfc_initiator_transceive_bytes(device, sddReqCl1, sddReqCl1Size, responseData, responseDataSize, 100);

  const uint8_t cascadeTag = 0x88;
  if (responseData[0] == cascadeTag) {
    printf("Found Cascade level 2 tag\n");
  } else {
    error("Got unknown result\n");
  }

  const int uidSize = 7;
  uint8_t uid[uidSize] = { responseData[1], responseData[2], responseData[3] };
  uint8_t bcc[2] = { responseData[4] };

  printf("Sending SEL_REQ CL1\n");
  const int selReqCL1Size = 9;
  uint8_t selReqCL1[selReqCL1Size] = { 0x93, 0x70 };
  memcpy(selReqCL1 + 2, responseData, 5);
  iso14443a_crc_append(selReqCL1, selReqCL1Size - 2);

  nfc_initiator_transceive_bytes(device, selReqCL1, selReqCL1Size, responseData, responseDataSize, 100);

  if (responseData[0] == 0x04) {
    printf("Got SAK for incomplete UID\n");
  } else {
    printf("Got unknown result\n");
    return -1;
  }

  printf("Sending SDD_REQ CL2\n");
  const int sddReqCL2Size = 2;
  const uint8_t sddReqCL2[sddReqCL2Size] = { 0x95, 0x20 };
  responseSize = nfc_initiator_transceive_bytes(device, sddReqCL2, sddReqCL2Size, responseData, responseDataSize, 100);
  if (responseSize == 5) {
    printf("Got the rest of UID\n");
  } else {
    error("Got unknown result\n");
  }

  memcpy(uid + 3, responseData, 4);
  bcc[1] = responseData[4];

  printf("UID: \n");
  printHex(uid, 7);
  printf("BCC: \n");
  printHex(bcc, 2);

}
