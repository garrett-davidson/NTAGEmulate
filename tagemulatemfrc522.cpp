#include "tagemulatemfrc522.h"

#include "logger.h"
#include "mfrc522.h"

#include <stdio.h>

MFRC522 *device;
int main(int argc, char **argv) {
  device = new MFRC522("/dev/spidev0.0");

  device->writeRegister(MFRC522::RegisterTModeReg, 0x8D); // Do I need this?
  device->writeRegister(MFRC522::RegisterTPrescalerReg, 0x3E); // Or this?
  device->writeRegister(MFRC522::RegisterTReloadRegL, 30);     // Or this?
  device->writeRegister(MFRC522::RegisterTReloadRegH, 0);      // Or this?

  device->writeRegister(MFRC522::RegisterTxASKReg, 0x40); // Force 100% ASK
  device->writeRegister(MFRC522::RegisterModeReg,
                        0 << 7 | // MSBFirst in CRC calculation
                        0 << 6 | // Reserved
                        1 << 5 | // TxWaitRF "transmitter can only be started if an RF field is generated"
                        0 << 4 | // Reserved
                        1 << 3 | // PolMFin set polarity of pin MFIN
                        0 << 2 | // Reserved
                        1 << 0   // CRCPreset[1:0] (01 = 0x6363)
                        );

  device->writeRegister(MFRC522::RegisterTxModeReg,
                        0 << 7 | // TxCRCEn
                        0 << 4 | // TxSpeed[2:0] (000 = 106kBd)
                        0 << 3   // InvMode
                        // Reserved
                        );

  device->writeRegister(MFRC522::RegisterRxModeReg,
                        0 << 7 | // RxCRCEn
                        0 << 4 | // RxSpeed[2:0] (000 = 106kBd)
                        1 << 3 | // RxNoErr
                        0 << 2   // RxMultiple
                        // Reserved
                        );

  device->writeRegister(MFRC522::RegisterCommandReg, 1<<3); // Receive

  uint8_t x = 0x14;
  while (x == 0x14) {
    device->readRegister(MFRC522::RegisterCommIrqReg, &x);
  }

  printf("%02X\n", x);
}
