#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_gatts.h"
/* ***************************************************************************** */
#define ESTC_UUID_BASE        {0xd5, 0x79, 0x5c, 0xb6, 0xbb, 0x30, 0x11, 0xec, \
                              0x84, 0x22, 0x02, 0x42, 0xAB, 0x7E, 0x17, 0xAC}

#define ESTC_UUID_SERVICE   0x16C1
#define ESTC_UUID_CHAR_1    0x16C2
/* ***************************************************************************** */
/** @brief estc ble service instance */
typedef struct
{
    uint16_t                    service_handle;
    uint8_t                     uuid_type;
    ble_gatts_char_handles_t    char1_handle;
    uint16_t                    desc_handle;
} ble_estc_service_t;

/**
 * @brief estc module initialization 
 *
 * @param service   pointer to ESTC service instance
 * @return true if OK, false if error
 */
bool estc_ble_service_init(ble_estc_service_t *service);

#endif /* ESTC_SERVICE_H__ */