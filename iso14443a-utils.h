#include <stdlib.h>

#ifdef linux
#include <stdint.h>
#endif

uint16_t iso14443a_crc(const uint8_t *data, size_t dataSize);
