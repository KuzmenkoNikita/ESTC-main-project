#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_gatts.h"
#include "ble.h"
/* ***************************************************************************** */
#define ESTC_SERVICE_BLE_OBSERVER_PRIO      3
/* ***************************************************************************** */
#define BLE_ESTC_SERVICE_DEF(_name)                                                                 \
static ble_estc_service_t _name;                                                                    \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     ESTC_SERVICE_BLE_OBSERVER_PRIO,                                                \
                     estc_service_on_ble_evt, &_name)
/* ***************************************************************************** */
#define ESTC_UUID_BASE        {0xd5, 0x79, 0x5c, 0xb6, 0xbb, 0x30, 0x11, 0xec, \
                              0x84, 0x22, 0x03, 0x42, 0xAB, 0x7E, 0x17, 0xAC}

#define ESTC_UUID_SERVICE       0x16C1
#define ESTC_UUID_CHAR_LED_H    0x16C2
#define ESTC_UUID_CHAR_LED_S    0x16C3
#define ESTC_UUID_CHAR_LED_V    0x16C4
/* ***************************************************************************** */
/**
 * @brief characteristic write callback
 *
 * @param char_uuid     char uuiid
 * @param write_val     value
 * @param p_ctx         contex passed to callback
 */
typedef void (*char_write_callback) (uint16_t char_uuid, uint32_t write_val, void* p_ctx);

/** @brief estc ble service instance */
typedef struct
{
    uint16_t                    service_handle;
    uint8_t                     uuid_type;
    ble_gatts_char_handles_t    char_led_h_handle;
    ble_gatts_char_handles_t    char_led_s_handle;
    ble_gatts_char_handles_t    char_led_v_handle;
    char_write_callback         fn_char_write_cb;
    void*                       p_ctx;
} ble_estc_service_t;

/** @brief estc ble service init param */
typedef struct 
{
    char_write_callback fn_char_write_callback;
    void*               p_ctx;
}ble_estc_service_info;


/**
 * @brief estc module initialization 
 *
 * @param service   pointer to ESTC service instance
 * @return true if OK, false if error
 */
bool estc_ble_service_init(ble_estc_service_t *service, ble_estc_service_info* p_init_info);

/**
 * @brief calback for handling service events
 */
void estc_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**
 * @brief notify characteristic
 *
 * @param conn_handle   Handle of the peripheral connection
 * @param service       pointer to ESTC service instance
 * @param char_uuid     char uuiid
 * @param value         char value to indicate
 */
bool estc_service_notify_char(uint16_t conn_handle, ble_estc_service_t *service, uint16_t char_uuid, uint16_t value);

#endif /* ESTC_SERVICE_H__ */