#include "logger.h"

#include <nfc/nfc.h>
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

  nfc_init(&context);
  device = nfc_open(context, NULL);

  if (!device) {
    error("Could not open NFC device\n");
  }

  nfc_target fakeTarget = {
    .nm = {
      .nmt = NMT_ISO14443A,
      .nbr = NBR_106,
    },
    .nti = {
      .nai = {
        .abtAtqa = { 0x00, 0x04 },
        .abtUid = { 0x08, 0xad, 0xbe, 0xef },
        .btSak = 0x00,
        .szUidLen = 4,
        .szAtsLen = 0,
      },
    },
  };

  const int responseDataSize = 300;
  uint8_t responseData[responseDataSize];
  int responseSize = 0;

  if (nfc_target_init(device, &fakeTarget, responseData, responseDataSize, 0) < 0) {
    error("Could not escape emulation\n");
  }

  printf("Escaped!\n");

  if (nfc_device_set_property_bool(device, NP_HANDLE_CRC, false) < 0) {
    error("Could not disbale CRC\n");
  }

  if (nfc_device_set_property_bool(device, NP_HANDLE_PARITY, true) < 0) {
    error("Could not enable parity\n");
  }

  do {
    sleep(1);
    responseSize = nfc_target_receive_bits(device, responseData, 7, NULL);
  } while (!responseSize || responseSize == -10);

  if (responseSize == 7 && responseData[0] == 0x26) {
    printf("Got REQA\n");
  } else {
    error("Got unknown response\n");
  }


}
