#include "mfrc522.h"

#include "logger.h"

#include <bcm2835.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#define BCM_RESET_PIN 25

bool MFRC522::bcmInit() {
  if (!bcm2835_init()) {
    printf("Could not init bcm\n");
    return false;
  }

  bcm2835_gpio_fsel(BCM_RESET_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_write(BCM_RESET_PIN, LOW);
  return true;
}

bool MFRC522::spiSetup() {
  bcm2835_spi_begin();
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);    // ~ 4 MHz
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
  return true;
}

MFRC522::MFRC522(const char *busName) {
  if (!bcmInit() || !spiSetup()) {
    printf("Could not set up SPI\n");
    printf("Try sudo\n");
    exit(1);
  }

  printf("Opened device\n");
}

void MFRC522::reset() {
  printf("Reseting\n");
  writeRegister(RegisterCommandReg, CommandSoftReset);
  uint8_t x;
  do {
    x = readRegister(RegisterCommandReg);
  } while (x & (1 << 4));
}

void MFRC522::setUp() {
  if (bcm2835_gpio_lev(BCM_RESET_PIN) == LOW) {
    printf("Powering up chip\n");
    bcm2835_gpio_write(BCM_RESET_PIN, HIGH);
    usleep(50000); // wait 50ms for chip to power up
  } else {
    reset();
  }

  // Set up timer
  writeRegister(MFRC522::RegisterTModeReg, 0x80);
  writeRegister(MFRC522::RegisterTPrescalerReg, 0xA9);
  writeRegister(MFRC522::RegisterTReloadRegL, 0x03);
  writeRegister(MFRC522::RegisterTReloadRegH, 0xE8);

  writeRegister(MFRC522::RegisterTxASKReg, 0x40); // Force 100% ASK
  writeRegister(MFRC522::RegisterModeReg, 0x3D);

  uint8_t temp = readRegister(RegisterTxControlReg);
  if (!(temp & 0x03))
    setBitMask(RegisterTxControlReg, 0x03);
  printf("Finished setup\n");
}

void MFRC522::writeRegister(const Register registerAddress, const uint8_t registerValue) {
  log(LogChannelSPI, "Writing %s: %02X\n", MFRC522RegisterNames[registerAddress], registerValue);
  uint8_t transmitBuffer[2] = {
    (uint8_t)(registerAddress << 1), // MSB = 0 for write, LSB = 0 always
    registerValue
  };

  uint8_t receiveBuffer[2];
  bcm2835_spi_transfernb((char*)transmitBuffer, (char*)receiveBuffer, 2);
}

uint8_t MFRC522::readRegister(const Register registerAddress) {
  uint8_t transmitBuffer[2] = {
    (uint8_t)((registerAddress << 1) | 0x80), // MSB = 1 for read, LSB = 0 always
    0
  };
  uint8_t receiveBuffer[2];
  bcm2835_spi_transfernb((char*)transmitBuffer, (char*)receiveBuffer, 2);

  log(LogChannelSPI, "Read %s: %02X\n", MFRC522RegisterNames[registerAddress], receiveBuffer[1]);

  return receiveBuffer[1];
}

void MFRC522::clearBitMask(Register registerAddress, uint8_t mask) {
  uint8_t x = readRegister(registerAddress);
  x &= ~mask;
  writeRegister(registerAddress, x);
}

void MFRC522::setBitMask(Register registerAddress, uint8_t mask) {
  uint8_t x = readRegister(registerAddress);
  x |= mask;
  writeRegister(registerAddress, x);
}






}
