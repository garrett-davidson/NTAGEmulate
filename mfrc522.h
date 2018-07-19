#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stddef.h>

#define BITS_TO_BYTES(x) ((x/8) + ((x % 8) ? 1 : 0))

class MFRC522 {
 public:
  enum Register {
    RegisterReserved00 = 0x00,
    RegisterCommandReg = 0x01,
    RegisterCommIEnReg = 0x02,
    RegisterDivlEnReg = 0x03,
    RegisterCommIrqReg = 0x04,
    RegisterDivIrqReg = 0x05,
    RegisterErrorReg = 0x06,
    RegisterStatus1Reg = 0x07,
    RegisterStatus2Reg = 0x08,
    RegisterFIFODataReg = 0x09,
    RegisterFIFOLevelReg = 0x0A,
    RegisterWaterLevelReg = 0x0B,
    RegisterControlReg = 0x0C,
    RegisterBitFramingReg = 0x0D,
    RegisterCollReg = 0x0E,
    RegisterReserved01 = 0x0F,
    RegisterReserved10 = 0x10,
    RegisterModeReg = 0x11,
    RegisterTxModeReg = 0x12,
    RegisterRxModeReg = 0x13,
    RegisterTxControlReg = 0x14,
    RegisterTxASKReg = 0x15,
    RegisterTxSelReg = 0x16,
    RegisterRxSelReg = 0x17,
    RegisterRxThresholdReg = 0x18,
    RegisterDemodReg = 0x19,
    RegisterReserved11 = 0x1A,
    RegisterReserved12 = 0x1B,
    RegisterMifareReg = 0x1C,
    RegisterReserved13 = 0x1D,
    RegisterReserved14 = 0x1E,
    RegisterSerialSpeedReg = 0x1F,
    RegisterReserved20 = 0x20,
    RegisterCRCResultRegM = 0x21,
    RegisterCRCResultRegL = 0x22,
    RegisterReserved21 = 0x23,
    RegisterModWidthReg = 0x24,
    RegisterReserved22 = 0x25,
    RegisterRFCfgReg = 0x26,
    RegisterGsNReg = 0x27,
    RegisterCWGsPReg = 0x28,
    RegisterModGsPReg = 0x29,
    RegisterTModeReg = 0x2A,
    RegisterTPrescalerReg = 0x2B,
    RegisterTReloadRegH = 0x2C,
    RegisterTReloadRegL = 0x2D,
    RegisterTCounterValueRegH = 0x2E,
    RegisterTCounterValueRegL = 0x2F,
    RegisterReserved30 = 0x30,
    RegisterTestSel1Reg = 0x31,
    RegisterTestSel2Reg = 0x32,
    RegisterTestPinEnReg = 0x33,
    RegisterTestPinValueReg = 0x34,
    RegisterTestBusReg = 0x35,
    RegisterAutoTestReg = 0x36,
    RegisterVersionReg = 0x37,
    RegisterAnalogTestReg = 0x38,
    RegisterTestDAC1Reg = 0x39,
    RegisterTestDAC2Reg = 0x3A,
    RegisterTestADCReg = 0x3B,
  };

  enum Command {
    CommandIdle = 0b0000,
    CommandMem = 0b0001,              /* Store 25 bytes in internal buffer */
    CommandGenerateRandomID = 0b0010, /* Generate 10-byte random id number */
    CommandCalcCRC = 0b0011,           /* Active CRC coprocessor and self-test */
    CommandTransmit = 0b0100,          /* Transmits from FIFO */
    CommandNoCmdChange = 0b0111,      /* Modify CommandReg without updating command (e.g. PowerDown) */
    CommandReceive = 0b1000,          /* Activate receiver */
    CommandTransceive = 0b1100,       /* Transmit then automatically receive */
    CommandMFAuthent = 0b1110,        /* Perform MIFARE auth as reader */
    CommandSoftReset = 0b1111,        /* Reset MFRC522 */
  };

  // ComIrqReg interrupt requests
  enum ComIrq {
    ComIrqSet1 = 1 << 7,
    ComIrqTxIrq = 1 << 6,       /* Set after last bit transmitted */
    ComIrqRxIrq = 1 << 5,       /* Set after end of valid receive stream */
    ComIrqIdleIrq = 1 << 4,     /* Unexpectedly set to idle */
    ComIrqHiAltertIrq = 1 << 3,       /* FIFO is getting full */
    ComIrqLoAltertIrq = 1 << 2,       /* FIFO is getting empty */
    ComIrqErrIrq = 1 << 1,            /* Error happened (check ErrorReg) */
    ComIrqTimerIrq = 1 << 0,          /* TCounterValReg hit 0 */
  };

  MFRC522(const char* busName);
  void reset();
  void setUp();
  uint8_t readRegister(const Register registerAddress);
  void writeRegister(const Register registerAddress, const uint8_t registerValue);
  int transceiveBits(const uint8_t *transmitData, const size_t transmitSizeBits, uint8_t *receiveData, const size_t receiveSizeBits, const size_t timeout);

  void clearBitMask(Register registerAddress, uint8_t mask);
  void setBitMask(Register registerAddress, uint8_t mask);

 private:
  int fd;
  bool bcmInit();
  bool spiSetup();
  struct spi_ioc_transfer spiBuffer;
};

static const char *MFRC522RegisterNames[] = {
    "Reserved00",
    "CommandReg",
    "CommIEnReg",
    "DivlEnReg",
    "CommIrqReg",
    "DivIrqReg",
    "ErrorReg",
    "Status1Reg",
    "Status2Reg",
    "FIFODataReg",
    "FIFOLevelReg",
    "WaterLevelReg",
    "ControlReg",
    "BitFramingReg",
    "CollReg",
    "Reserved01",
    "Reserved10",
    "ModeReg",
    "TxModeReg",
    "RxModeReg",
    "TxControlReg",
    "TxASKReg",
    "TxSelReg",
    "RxSelReg",
    "RxThresholdReg",
    "DemodReg",
    "Reserved11",
    "Reserved12",
    "MifareReg",
    "Reserved13",
    "Reserved14",
    "SerialSpeedReg",
    "Reserved20",
    "CRCResultRegM",
    "CRCResultRegL",
    "Reserved21",
    "ModWidthReg",
    "Reserved22",
    "RFCfgReg",
    "GsNReg",
    "CWGsPReg",
    "ModGsPReg",
    "TModeReg",
    "TPrescalerReg",
    "TReloadRegH",
    "TReloadRegL",
    "TCounterValueRegH",
    "TCounterValueRegL",
    "Reserved30",
    "TestSel1Reg",
    "TestSel2Reg",
    "TestPinEnReg",
    "TestPinValueReg",
    "TestBusReg",
    "AutoTestReg",
    "VersionReg",
    "AnalogTestReg",
    "TestDAC1Reg",
    "TestDAC2Reg",
    "TestADCReg",
    "Reserved31",
    "Reserved32",
    "Reserved33",
    "Reserved34",
  };
