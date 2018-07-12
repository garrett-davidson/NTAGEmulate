#include <stdlib.h>

#ifdef linux
#include <stdint.h>
#endif

uint16_t iso14443aCRC(const uint8_t *data, size_t dataSize);
void iso14443aCRCAppend(uint8_t *data, size_t dataSize);
