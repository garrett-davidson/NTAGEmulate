#include "mfrc522.h"

#include "logger.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

MFRC522::MFRC522(const char *busName) {
  if ((fd = open(busName, O_RDWR)) < 0) {
    printf("Failed to open the SPI bus.\n");
    perror("SPI:");
    exit(-1);
  }
  int mode = SPI_MODE_0, speed = 500000, lsb = 0, bits = 8;

  if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
    perror("SPI wr_mode");
    return;
  }

  if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0) {
    perror("SPI rd_mode");
    return;
  }

  if (ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb) < 0) {
    perror("SPI wr_lsb_fist");
    return;
  }

  if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
    perror("SPI rd_lsb_fist");
    return;
  }

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
    perror("SPI wr_bits_per_word");
    return;
  }

  if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
    perror("SPI rd_bits_per_word");
    return;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
    perror("SPI wr_max_speed_hz");
    return;
  }

  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
    perror("SPI rd_max_speed_hz");
    return;
  }

  printf("%s: spi mode %d, %d bits %sper word, %d Hz max\n", busName, mode, bits, lsb ? "" : "(msb first) ", speed);

  spiBuffer.cs_change = 0;
  spiBuffer.delay_usecs = 0;
  spiBuffer.speed_hz = speed;
  spiBuffer.bits_per_word = 8;
}

int MFRC522::writeRegister(const Register registerAddress, const uint8_t registerValue) {
  log(LogChannelSPI, "Writing %s: %02X\n", MFRC522RegisterNames[registerAddress], registerValue);
  const uint8_t transmitBuffer[2] = { (uint8_t)(registerAddress << 1), registerValue };

  uint8_t receiveBuffer[2];
  int status = transceive(transmitBuffer, receiveBuffer, 2);

  return status;
}

int MFRC522::readRegister(const Register registerAddress, uint8_t *registerValue) {
  const uint8_t transmitBuffer[2] = { (uint8_t)((registerAddress << 1) | 0x80), 0 };
  uint8_t receiveBuffer[2];
  int status = transceive(transmitBuffer, receiveBuffer, 2);

  *registerValue = receiveBuffer[1];

  log(LogChannelSPI, "Read %s: %02X\n", MFRC522RegisterNames[registerAddress], receiveBuffer[1]);

  return status;
}

int MFRC522::transceive(const uint8_t *inData, uint8_t *outData, const size_t length) {

  const int bufferSize = sizeof(__u64); // sizeof(spi_ioc_transfer.tx_buf)
  uint8_t txBuffer[bufferSize];
  uint8_t rxBuffer[bufferSize];
  memset(txBuffer, 0, bufferSize);
  memset(rxBuffer, 0, bufferSize);

  memcpy(txBuffer, inData, length);

  spiBuffer.len = length;
  spiBuffer.tx_buf = (unsigned long)txBuffer;
  spiBuffer.rx_buf = (unsigned long)rxBuffer;

  int status = ioctl(fd, SPI_IOC_MESSAGE(1), &spiBuffer);

  memcpy(outData, rxBuffer, length);

  return status;
}
