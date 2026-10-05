/* crc32.c - bitwise CRC-32 (IEEE). No lookup table, so no flash/RAM cost. */
#include "crc32.h"

uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;

    while (len--) {
        crc ^= *data++;
        for (uint32_t bit = 0; bit < 8u; bit++) {
            /* if LSB set: shift and XOR polynomial, else just shift */
            crc = (crc >> 1) ^ (0xEDB88320UL & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}
