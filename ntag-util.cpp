#include <nfc/nfc.h>
#include <stdlib.h>
#include <unistd.h>

nfc_context *context;
nfc_device *device;

static void cleanup(int) {
  if (device != NULL)
    nfc_abort_command(device);

  nfc_exit(context);
  exit(EXIT_FAILURE);
}

void printHex(const uint8_t buffer[], int size) {
  for (int i = 0; i < size; i++) {
      printf("%02X ", buffer[i]);
  }

  printf("\n");
}

enum NTAGCommand {
  CommandGetVersion = 0x60,
  CommandRead = 0x30,
  CommandReadSig = 0x3C,
};

void readSig() {
  const uint8_t command[] = {
    CommandReadSig,
    0x00,                       // Address (RFU)
  };

  const int responseSize = 32 + 2; // Signature + CRC
  uint8_t responseBuffer[responseSize];

  if (nfc_initiator_transceive_bytes(device, command, sizeof(command), responseBuffer, responseSize, 100) < 0) {
    nfc_perror(device, "ReadSig");
    return;
  }

  printf("Signature:\n");
  printHex(responseBuffer, responseSize);
}

int main(int argc, char **argv) {
  signal(SIGINT, cleanup);

  nfc_init(&context);
  if (!context) {
    printf("Could not init context\n");
    return -1;
  }

  device = nfc_open(context, NULL);
  if (!device) {
    printf("Could not open device\n");
    return -1;
  }

  if (nfc_initiator_init(device) < 0) {
    printf("Could not initiate device\n");
    return -1;
  }

  nfc_modulation nm = {
    .nmt = NMT_ISO14443A,
    .nbr = NBR_106,
  };

  nfc_target target;

  printf("Looking for target\n");
  while (nfc_initiator_select_passive_target(device, nm, NULL, 0, &target) < 0) {
    sleep(1);
    printf("Still looking\n");
  }

  nfc_device_set_property_bool(device, NP_EASY_FRAMING, false);

  readSig();
}
