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

#define ESTC_UUID_CHAR_SET_LED_RGB          0x16C2
#define ESTC_UUID_CHAR_LED_COLOR_CHANGED    0x16C3

/* ***************************************************************************** */
/**
 * @brief characteristic write callback
 *
 * @param R             Red color component 
 * @param G             Green color component  
 * @param B             Blue color component 
 * @param p_ctx         contex passed to callback
 */
typedef void (*led_color_write_callback) (uint8_t R, uint8_t G, uint8_t B, void* p_ctx);


/** @brief estc ble service instance */
typedef struct
{
    uint16_t                    service_handle;
    uint8_t                     uuid_type;
    ble_gatts_char_handles_t    char_led_color_changed;
    ble_gatts_char_handles_t    char_set_led_rgb;
    led_color_write_callback    fn_char_write_cb;
    void*                       p_ctx;
} ble_estc_service_t;


/** @brief estc ble service init param */
typedef struct 
{
    led_color_write_callback    fn_char_write_callback;
    void*                       p_ctx;
}ble_estc_service_info;


/**
 * @brief estc module initialization 
 *
 * @param service   pointer to ESTC service instance
 * @param p_init_info module init params
 * @return true if OK, false if error
 */
bool estc_ble_service_init(ble_estc_service_t *service, ble_estc_service_info* p_init_info);

/**
 * @brief calback for handling service events
 */
void estc_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**
 * @brief send characteristic value 
 *
 * @param conn_handle   Handle of the peripheral connection
 * @param service       pointer to ESTC service instance
 * @param R             Red color component 
 * @param G             Green color component  
 * @param B             Blue color component  
 */
bool estc_service_led_color_change(uint16_t conn_handle, ble_estc_service_t *service, 
                                uint8_t R, uint8_t G, uint8_t B);

#endif /* ESTC_SERVICE_H__ */