#include <nfc/nfc.h>
#include <stdio.h>
#include <stdlib.h>

nfc_context *context;
nfc_device *device;

bool shouldQuit = false;
bool shouldPoll = true;

static void stop_polling(int sig)
{
  shouldQuit = true;
  (void) sig;
  if (device != NULL)
    nfc_abort_command(device);
  else {
    nfc_exit(context);
    exit(-1);
  }
}

void printTarget(const nfc_target *target) {
  printf("Found target\n");
  char *targetString;
  str_nfc_target(&targetString, target, true);
  printf("%s\n", targetString);
  nfc_free(targetString);
}

void poll() {
  const nfc_modulation nmModulations[1] = {
    { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
  };
  const size_t szModulations = 1;

  nfc_target target;
  printf("Polling\n");
  int res;
  if ((res = nfc_initiator_poll_target(device, nmModulations, szModulations, 0xFF, 1, &target)) < 0) {
    printf("Polling error\n");
  } else {
    printTarget(&target);
  }
}

void listPassiveTargets() {
  const nfc_modulation nmModulation =
    { .nmt = NMT_ISO14443A, .nbr = NBR_106 };
  nfc_target target;

  int res;
  if ((res = nfc_initiator_list_passive_targets(device, nmModulation, &target, 1)) < 0) {
    printf("Listing error\n");
  } else if (!res) { return; }
  else {
    printTarget(&target);
  }
}

int main(int argc, char **argv) {
  signal(SIGINT, stop_polling);

  nfc_init(&context);

  const char *connString = NULL;
  if (argc == 2) {
    connString = argv[1];
  }
  device = nfc_open(context, connString);

  nfc_initiator_init(device);

  while (!shouldQuit) {
    if (shouldPoll) {
      poll();
    } else {
      listPassiveTargets();
    }
  }

  nfc_close(device);
  nfc_exit(context);
}
