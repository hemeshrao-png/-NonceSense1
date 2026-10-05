/* crc32.h - standard CRC-32 (IEEE 802.3, same as zlib.crc32 / Python binascii) */
#ifndef CRC32_H
#define CRC32_H
#include <stdint.h>

/* Bitwise, table-free implementation: poly 0xEDB88320 (reflected),
 * init 0xFFFFFFFF, final XOR 0xFFFFFFFF. Small and easy to audit. */
uint32_t crc32_calc(const uint8_t *data, uint32_t len);

#endif
