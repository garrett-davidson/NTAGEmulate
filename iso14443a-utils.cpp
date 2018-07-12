#include "iso14443a-utils.h"

#include <stdio.h>

uint16_t iso14443a_crc(const uint8_t *pbtData, size_t szLen) {
  uint32_t wCrc = 0x6363;

  do {
    uint8_t  bt;
    bt = *pbtData++;
    bt = (bt ^ (uint8_t)(wCrc & 0x00FF));
    bt = (bt ^ (bt << 4));
    wCrc = (wCrc >> 8) ^ ((uint32_t) bt << 8) ^ ((uint32_t) bt << 3) ^ ((uint32_t) bt >> 4);
  } while (--szLen);

  return (uint16_t)wCrc;
}
