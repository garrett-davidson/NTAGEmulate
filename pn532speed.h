#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HIGH(x) (uint8_t)(x >> 8)
#define LOW(x) (uint8_t)(x & 0xFF)
#define REG(x) HIGH(x), LOW(x)

enum Registers {
  RegisterCFR = 0x6200,
  RegisterCIU_Mode = 0x6301,
  RegisterCIU_TxMode = 0x6302,
  RegisterCIU_RxMode = 0x6303,
  RegisterCIU_ManualRCV = 0x630D,
  RegisterCIU_Command = 0x6331,
  RegisterCIU_CommIEn = 0x6332,
  RegisterCIU_CommIrq = 0x6334,
  RegisterCIU_Error = 0x6336,
  RegisterCIU_Status2 = 0x6338,
  RegisterCIU_FIFOData = 0x6339,
  RegisterCIU_FIFOLevel = 0x633A,
  RegisterCIU_Control = 0x633C,
  RegisterCIU_BitFraming = 0x633D,
};

extern bool shouldQuit;

void printHex(const uint8_t buffer[], int size);

bool openSerialPort(const char *portName);

bool wakeUp();
int pn532SetUp();
int sendFrame(const uint8_t *frame, const size_t frameLength, uint8_t *responseBuffer, const size_t responseBufferSize);

inline void writeRegister(const uint16_t registerAddress, const uint8_t value) {
  const uint8_t high = (uint8_t)(registerAddress >> 8);
  const uint8_t low = (uint8_t)(registerAddress & 0xFF);
  const uint8_t writeRegisterCommand[] = {
    0x00,                       // Preamble
    0x00,
    0xFF,                       // Start of frame
    0x05,                       // Length
    0xFB,                       // Length checksum
    0xD4,                       // Direction
    0x08,                       // WriteRegister
    high,                       // Register address high
    low,                        // Register address low
    value,
    (uint8_t)(0 - 0xD4 - 0x08 - high - low - value), // DCS
    0x00,                       // Postamble
  };

  const int responseSize = 9;
  uint8_t responseFrame[responseSize];
  sendFrame(writeRegisterCommand, sizeof(writeRegisterCommand), responseFrame, responseSize);
}

inline uint8_t readRegister(uint16_t registerAddress) {
  const uint8_t high = (uint8_t)(registerAddress >> 8);
  const uint8_t low = (uint8_t)(registerAddress & 0xFF);
  const uint8_t readRegisterCommand[] = {
    0x00,                       // Preamble
    0x00,
    0xFF,                       // Start of frame
    0x04,                       // Length
    0xFC,                       // Length checksum
    0xD4,                       // Direction
    0x06,                       // ReadRegister
    high, // Register address high
    low, // Register address low
    (uint8_t)(0 - 0xD4 - 0x06 - high - low), // DCS
    0x00,                       // Postamble
  };

  const int responseSize = 10;
  uint8_t responseFrame[responseSize];
  if (sendFrame(readRegisterCommand, sizeof(readRegisterCommand), responseFrame, responseSize) < 0) {
    return 0;
  }

  return responseFrame[7];
}

inline uint8_t awaitReceive() {
  const uint8_t interrupts =
  1 << 5                        // RxIEn
  // | 1 << 1                      // ErrIEn
  ;

  uint8_t irq = 0;
  while (!(irq & interrupts) && !shouldQuit) {
    irq = readRegister(RegisterCIU_CommIrq);
  }

  return irq;
}

inline void dumpFIFO() {
  writeRegister(RegisterCIU_FIFOLevel, 1 << 7);
}

inline void clearInterrupts() {
  writeRegister(RegisterCIU_CommIrq, 0x7F);
}

void printFullFIFO();
