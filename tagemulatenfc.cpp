#include "logger.h"

#include <nfc/nfc.h>
#include <unistd.h>

nfc_context *context;
nfc_device *device;

void error(const char* errorMessage) {
  printf("%s\n", errorMessage);
  nfc_perror(device, "Error:");
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

  const uint8_t uid[7] = { 0x04, 0x04, 0xCD, 0x6A, 0xC5, 0x58, 0x81 };
  const uint8_t bcc[2] = { 0x45, 0x76 };

  const int atqaSize = 2;
  const uint8_t atqa[atqaSize] = { 0x44, 0x00 };

  const int sddResCL1Size = 5;
  const uint8_t sddReqCL1[sddResCL1Size] = { 0x88, uid[0], uid[1], uid[2], bcc[0] };

  const int sakCL1Size = 3;
  uint8_t sakCL1[sakCL1Size] = { 0x04 };
  iso14443a_crc_append(sakCL1, 1);

  const int sddResCL2Size = 5;
  const uint8_t sddResCL2[sddResCL2Size] = { uid[3], uid[4], uid[5], uid[6], bcc[1] };

  if (nfc_device_set_property_bool(device, NP_EASY_FRAMING, false) < 0) {
    error("Could not disable easy framing\n");
  }

  if (nfc_device_set_property_bool(device, NP_HANDLE_CRC, false) < 0) {
    error("Could not disable CRC\n");
  }

  if (nfc_device_set_property_bool(device, NP_HANDLE_PARITY, true) < 0) {
    error("Could not enable parity\n");
  }

  responseSize = nfc_target_receive_bits(device, responseData, 7, NULL);
  int transmitSize;
  const uint8_t *transmitBits;
  int receiveSize;
  while (1) {
    switch (responseSize) {
    case 7: // REQA
      printf("REQA\n");
      transmitSize = atqaSize * 8;
      transmitBits = atqa;
      receiveSize = 16;
      break;

    case 16: // SDD_REQ CL1
      switch (*responseData) {
      case 0x93:
        printf("SDD_REQ CL1\n");
        transmitSize = sddResCL1Size * 8;
        transmitBits = sddReqCL1;
        receiveSize = 72;
        break;

      case 0x95: // SDD_REQ CL2
        printf("SDD_REQ CL2\n");
        transmitSize = sddResCL2Size * 8;
        transmitBits = sddResCL2;
        receiveSize = 9 * 8;
        break;

      default:
        printf("Unexpected value: %X\n", *responseData);
      }
      break;

    case 72: // SEL_REQ CL1
      printf("SEL_REQ CL1\n");
      transmitSize = sakCL1Size * 8;
      transmitBits = sakCL1;
      receiveSize = 16;
      break;

    case 0:  // No response?
    case -5: // Buffer overflow?
    case -6: // Timeout
    case NFC_ETGRELEASED: // RF Field gone
      break;

    default:
      printf("Received unknown size response: %d\n", responseSize);
      if (responseSize > 0) {
        printHex(responseData, (responseSize / 8) + ((responseSize % 8) ? 1 : 0));
      }
      return 0;
    }

    if (transmitSize) {
      printf("Sending\n");
      nfc_target_send_bits(device, transmitBits, transmitSize, NULL);
    }  else {
      printf("Not sending\n");
    }

    responseSize = nfc_target_receive_bits(device, responseData, responseDataSize, NULL);
    printf("Read %d\n", responseSize);
  }
}
