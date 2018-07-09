#include <stdlib.h>

struct sp_port;

class PN532 {
public:
  PN532(const char *portName);
  int wakeUp();
  int setUp();
  int sendCommand(const uint8_t *command, int commandSize, uint8_t *responseBuffer, const size_t responseBufferSize);
  int readTagId(uint8_t *idBuffer, uint8_t idBufferLength, uint8_t tagBaudRate);

  int ntag2xxReadPage(uint8_t page, uint8_t *buffer);

  void printHex(const uint8_t buffer[], int size);

  enum NFCParameters {
    fNADUsed = 1 << 0, // Use Network ADdress for initiator
    fDIDUsed = 1 << 1, // Use Device IDentifier (?) for initiator (or CID for ISO/IEC14443-4 PCD)
    fAutomaticATR_RES = 1 << 2, // Automatically generate ATR_RES for target
    RFU = 1 << 3, // Must always be 0
    fAutomaticRATS = 1 << 4, // Automatically generate RATS for ISO/IEC14443-4 PCD
    fISO14443 = 1 << 5, // Emulate ISO/IEC14443-4 PICC
    fRemovePrePostAmble = 1 << 6, // Do not send Pre/Postamble
    RFU2 = 1 << 7, // Must always be 0
  };

  // Tx = Host -> PN532 (transmit)
  // Rx = PN532 -> Host (receive)
  enum NFCCommands {
    TxGetFirmwareVersion = 0x02,
    RxGetFirmwareVersion = 0x03,
    TxReadRegister = 0x06,
    RxReadRegister = 0x07,
    TxSetParameters = 0x12,
    RxSetParameters = 0x13,
    TxSAMConfiguration = 0x14,
    RxSAMConfiguration = 0x15,
    TxInDataExchange = 0x40,
    RxInDataExchange = 0x41,
    TxInListPassiveTarget = 0x4A,
    RxInListPassiveTarget = 0x4B,
  };

  enum TargetBaudRates {
    TypeABaudRate = 0x00, // 106 kbps ISO/IEC14443 Type A
  };

  enum SamConfigurationMode {
    SamConfigurationModeNormal = 0x01,
    SamConfigurationModeVirtualCard = 0x02,
    SamConfigurationModeWiredCard = 0x03,
    SamConfigurationModeDualCard = 0x04,
  };

  enum MifareCommands {
    MifareReadPage = 0x30,
    MifareWritePage = 0xA0,
  };

private:
  struct sp_port *port;
  int awaitResponse(uint8_t *responseBuffer, int responseBufferSize);
  int awaitAck();
  int sendFrame(const uint8_t *data, int size);
  int samConfig(SamConfigurationMode mode, uint8_t timeout = 0);
  void printFrame(const uint8_t *frame, const size_t frameLength);
};
