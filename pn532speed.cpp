#include "pn532speed.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

bool shouldQuit = false;

#define HIGH(x) (uint8_t)(x >> 8)
#define LOW(x) (uint8_t)(x & 0xFF)
#define REG(x) HIGH(x), LOW(x)

int serialFd;

void printHex(const uint8_t buffer[], int size) {
  for (int i = 0; i < size; i++) {
      printf("%02X ", buffer[i]);
  }

  printf("\n");
}

static inline int serialWrite(const uint8_t *buffer, const size_t bufferSize) {
  return write(serialFd, buffer, bufferSize);
}

int readCount;
static inline int serialRead(uint8_t *buffer, const size_t bufferSize) {
  size_t readSize = 0;
  readCount = 0;
  while (readSize < bufferSize && !shouldQuit) {
    readSize += read(serialFd, buffer + readSize, bufferSize - readSize);

    if (++readCount > 5000000) {
      printf("Timeout\n");
      //exit(1);
      return readSize;
    }
  }

  return readSize;
}

bool openSerialPort(const char *portName) {
  serialFd = open(portName, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (serialFd == -1){
    printf("Could not open serial port\n");
    return false;
  };

  struct termios options;

  // Populate options with current settings
  tcgetattr(serialFd, &options);

  // Set I/O speed
  cfsetspeed(&options, B115200);

  // Set Data bits
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;

  // Disable parity
  options.c_cflag &= ~PARENB;

  // Stop bits default to 1, but set it again anyway
  options.c_cflag &= ~CSTOPB;

  // Ignore modem control lines
  options.c_cflag |= CLOCAL;

  // Enable read
  options.c_cflag |= CREAD;

  // Set communication to raw mode
  //cfmakeraw(&options);
  options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  options.c_oflag &= ~OPOST;
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  options.c_cflag &= ~(CSIZE | PARENB);
  options.c_cflag |= CS8;

  // Nonblocking read
  options.c_cc[VTIME] = 0;
  options.c_cc[VMIN] = 0;

  // Set our new options
  return tcsetattr(serialFd, TCSANOW, &options) == 0;
}

bool wakeUp() {
  const int wakeBufferSize = 16;
  uint8_t wakeBuffer[wakeBufferSize] = { 0x55, 0x55, 0x00, 0x00, 0x00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 };

  int error;
  if ((error = serialWrite(wakeBuffer, wakeBufferSize)) != wakeBufferSize) {
    printf("Error waking: %d\n", error);
    return false;
  }

  return true;
}

int sendFrame(const uint8_t *frame, const size_t frameLength, uint8_t *responseBuffer, const size_t responseBufferSize) {
  // Send frame
  if ((size_t)serialWrite(frame, frameLength) != frameLength) {
    printf("Failed to send frame\n");
    return -1;
  }

  // Get ACK
  uint8_t ackBuffer[6];
  const int ackBufferLength = sizeof(ackBuffer);
  size_t x;
  if ((x = serialRead(ackBuffer, ackBufferLength)) != ackBufferLength) {
    printf("Failed to get ACK: %ld\n", x);
    return -1;
  }

  // Get response
  x = 0;
  while (x < responseBufferSize && !shouldQuit) {
    x += serialRead(responseBuffer, responseBufferSize);
    if (x < 0) {
      printf("Read error\n");
    }
  }

  return x;
}

const uint8_t samConfigCommand[] = {
  0x00,                         // Preamble
  0x00,
  0xFF,                         // Start of frame
  0x04,                         // Length
  (uint8_t)(0 - samConfigCommand[3]), // Length checksum
  0xD4,                         // Direction
  0x14,                         // SAMConfiguration
  0x01,                         // Normal mode
  0x00,                         // Timeout (unused in Normal mode)
  (uint8_t)(0 - 0xD4 - 0x14 - 0x01), // Data checksum
  0x00,                         // Postamble
};
const int samConfigResponseLength = 9;

int samConfig() {
  uint8_t responseBuffer[100];

  // Configure normal mode to get PN532 activated
  int x;
  if ((x = sendFrame(samConfigCommand, sizeof(samConfigCommand), responseBuffer, samConfigResponseLength)) != samConfigResponseLength) {
    printf("Could not configure SAM: %d\n", x);
    printHex(responseBuffer, samConfigResponseLength);
    return -1;
  }

  return 0;
}

int pn532SetUp() {
  return samConfig();
}

void printFullFIFO() {
  uint8_t fifoCount = readRegister(RegisterCIU_FIFOLevel);

  printf("FIFO count: %d\n", fifoCount);

  if (!fifoCount) return;

  for (int i = 0; i < fifoCount; i++) {
    printf("%02X ", readRegister(RegisterCIU_FIFOData));
  }

  printf("\n");
}

void watchFIFO(uint8_t fifoLevel, int printInterval) {
  uint8_t responseBuffer[500];

  uint8_t fifoData[500];
  int readFIFODataLength = 0;

  uint8_t currentFIFOLevel = 0;
  uint8_t fetchFIFOBuffer[140] = {
    0x00,                       // Preamble
    0x00,
    0xFF,                       // Start of frame
    0x00,                       // Length
    0x00,                       // Length checksum
    0xD4,                       // Direction
    0x06,                       // ReadRegister
  };
  memset(fetchFIFOBuffer + 7, 0, sizeof(fetchFIFOBuffer) - 7);

  writeRegister(RegisterCIU_WaterLevel, fifoLevel);

  const uint8_t fifoPattern[4] = {
    HIGH(RegisterCIU_FIFOData),
    LOW(RegisterCIU_FIFOData),
    HIGH(RegisterCIU_FIFOData),
    LOW(RegisterCIU_FIFOData)
  };
  while (!shouldQuit) {
    awaitInterrupts(InterruptHiAlertIrq);

    currentFIFOLevel = readRegister(RegisterCIU_FIFOLevel);

    // Set length
    // 2 bytes per register read + 1 for Direction + 1 for ReadRegister
    fetchFIFOBuffer[3] = (currentFIFOLevel * 2) + 2;

    // Set length checksum
    fetchFIFOBuffer[4] = (uint8_t)(0 - fetchFIFOBuffer[3]);

    // Set data checksum
    int dcsPosition = 7 + (2*currentFIFOLevel);
    fetchFIFOBuffer[dcsPosition] = (uint8_t)(0
                                             - fetchFIFOBuffer[5]
                                             - fetchFIFOBuffer[6]
                                             - (currentFIFOLevel * HIGH(RegisterCIU_FIFOData))
                                             - (currentFIFOLevel * LOW(RegisterCIU_FIFOData))
                                             );

    // Set postamble
    fetchFIFOBuffer[dcsPosition + 1] = 0;

    // Write RegisterCIU_FIFOData, currentFIFOLevel times
    uint16_t *bufferPointer = (uint16_t *)(fetchFIFOBuffer + 7);
    const uint16_t address = (LOW(RegisterCIU_FIFOData) << 8) | HIGH(RegisterCIU_FIFOData); // Swap the two address bytes
    for (int i = 0; i < currentFIFOLevel; i++) {
      bufferPointer[i] = address;
    }

    int fetchFIFOFrameLength = dcsPosition + 2;
    int responseSize = currentFIFOLevel + 9;
    sendFrame(fetchFIFOBuffer, fetchFIFOFrameLength, responseBuffer, responseSize);
    memcpy(fifoData + readFIFODataLength, responseBuffer + 7, currentFIFOLevel);
    readFIFODataLength += currentFIFOLevel;

    if (readFIFODataLength > 100) {
      printf("FIFO Data: %d\n", readFIFODataLength);
      printHex(fifoData, readFIFODataLength);
      readFIFODataLength = 0;
    }
  }

  if (!readFIFODataLength) return;

  // Print whatever data is left
  printf("FIFO Data: %d\n", readFIFODataLength);
  printHex(fifoData, readFIFODataLength);
}
