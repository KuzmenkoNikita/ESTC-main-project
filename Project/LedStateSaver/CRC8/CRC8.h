#ifndef CRC8_CALC
#define CRC8_CALC

#include <stdint.h>

/**
 * @brief Calculate CRC8
 *
 * @param pData     Pointer to data
 * @param len       data len
 * @return calculated checksum
 */
uint8_t CRC8_calc(uint8_t* pData, uint32_t len);

#endif /* CRC8_CALC */