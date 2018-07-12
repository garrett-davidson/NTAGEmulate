#include <stdlib.h>

#ifdef linux
#include <stdint.h>
#endif

struct sp_port;

class PN532 {
public:
  PN532(const char *portName);
  ~PN532();
  void close();

  enum SetupMode {
    InitiatorMode,
    TargetMode,
  };

  int wakeUp();
  int setUp(SetupMode mode);
  int sendCommand(const uint8_t *command, int commandSize, uint8_t *responseBuffer, const size_t responseBufferSize, int timeout);
  int readTagId(uint8_t *idBuffer, uint8_t idBufferLength, uint8_t tagBaudRate);
  int setParameters(uint8_t parameters);
  int initAsTarget(uint8_t mode, const uint8_t *mifareParams, uint8_t responseBuffer[], const size_t responseBufferSize);
  int getInitiatorCommand(uint8_t responseBuffer[], const size_t responseBufferSize);

  uint8_t readRegister(uint16_t registerAddress);
  int writeRegister(uint16_t registerAddress, uint8_t registerValue);

  int ntag2xxReadPage(uint8_t page, uint8_t *buffer);
  int ntag2xxEmulate(const uint8_t *uid, const uint8_t *data);

  void printHex(const uint8_t buffer[], int size);
  void printFrame(const uint8_t *frame, const size_t frameLength);

  int sendRawBitsInitiator(const uint8_t *bitData, const size_t bitCount, uint8_t *responseFrame, const size_t responseFrameSize);
  int sendRawBytesInitiator(const uint8_t *byteData, const size_t byteCount, uint8_t *responseFrame, const size_t responseFrameSize);

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
    TxWriteRegister = 0x08,
    RxWriteRegister = 0x09,
    TxSetParameters = 0x12,
    RxSetParameters = 0x13,
    TxSAMConfiguration = 0x14,
    RxSAMConfiguration = 0x15,
    TxRFConfiguration = 0x32,
    RxRFConfiguration = 0x33,
    TxInDataExchange = 0x40,
    RxInDataExchange = 0x41,
    TxInCommunicateThrough = 0x42,
    RxInCommunicateThrough = 0x43,
    TxInListPassiveTarget = 0x4A,
    RxInListPassiveTarget = 0x4B,
    TxTgGetData = 0x86,
    RxTgGetData = 0x87,
    TxTgInitAsTarget = 0x8C,
    RxTgInitAsTarget = 0x8D,
    TxTgGetInitiatorCommand = 0x88,
    RxTgGetInitiatorCommand = 0x89,
    TxTgResponseToInitiator = 0x90,
    RxTgResponseToInitiator = 0x91,
  };

  enum TargetBaudRates {
    TypeABaudRate = 0x00, // 106 kbps ISO/IEC14443 Type A
  };

  enum TargetModes {
    TargetModePassiveOnly = 1 << 0,
    TargetModeDEPOnly = 1 << 1,
    TargetModePICCOnly = 1 << 2,
  };

  enum SamConfigurationMode {
    SamConfigurationModeNormal = 0x01,
    SamConfigurationModeVirtualCard = 0x02,
    SamConfigurationModeWiredCard = 0x03,
    SamConfigurationModeDualCard = 0x04,
  };

  enum NTAG21xCommands {
    NTAG21xRequest = 0x26,
    NTAG21xReadPage = 0x30,
    NTAG21xWritePage = 0xA0,
    NTAG21xHalt = 0x50,
  };

  enum Registers {
    RegisterCIU_TxMode = 0x6302,
    RegisterCIU_RxMode = 0x6303,
    RegisterCIU_ManualRCV = 0x630D,
    RegisterCIU_Error = 0x6336,
    RegisterCIU_Control = 0x633C,
    RegisterCIU_BitFraming = 0x633D,
  };

private:
  struct sp_port *port;
  bool shouldQuit;

  static const size_t serialBufferSize = 500;
  uint8_t serialBuffer[serialBufferSize];
  size_t readSize;

  int getResponse(uint8_t *responseBuffer, int responseBufferSize, int timeout);
  int awaitAck();
  int sendFrame(const uint8_t *data, int size);
  int samConfig(SamConfigurationMode mode, uint8_t timeout = 0);
  int readSerialFrame(uint8_t *buffer, const size_t bufferSize, int timeout);
};
