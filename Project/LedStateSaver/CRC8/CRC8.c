
#include "CRC8.h"

uint8_t CRC8_calc(uint8_t* pData, uint32_t len)
{
    uint8_t crc = 0xFF;
    uint32_t i;

    while (len--)
    {
        crc ^= *pData++;

        for (i = 0; i < 8; i++)
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }

    return crc;
}