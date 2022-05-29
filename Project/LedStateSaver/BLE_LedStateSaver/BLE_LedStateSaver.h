#ifndef BLE_LEDSTATESAVER
#define BLE_LEDSTATESAVER

#include <stdint.h>
#include <stdbool.h>
#include "HSV_to_RGB_Calc.h"

#define BLE_LEDSTATESAVER_COUNTOF_PAGES 2

/** @brief module state */
typedef enum
{
    BLE_LEDSAVER_INIT_SUCCESSFUL,
    BLE_LEDSAVER_INIT_ERROR,
    BLE_LEDSAVER_WRITE_SUCCESSFUL,
    BLE_LEDSAVER_WRITE_ERROR,
}ble_ledsaver_state;

/**
 * @brief Button interrupt handler
 *
 * @param eState   button state
 * @param pData pointer to user data
 */
typedef void (*ble_ledstatesaver_state_changed)(ble_ledsaver_state state, void* p_data);

/** @brief module instance */
typedef struct 
{
    uint32_t                            pages_addrs[BLE_LEDSTATESAVER_COUNTOF_PAGES];
    uint32_t                            last_op_state[BLE_LEDSTATESAVER_COUNTOF_PAGES];
    uint32_t                            data_to_write;
    uint32_t                            write_addr;
    uint32_t                            read_addr;
    uint32_t                            active_page;
    ble_ledstatesaver_state_changed     state_changed_callback;
    void*                               p_data;     
}ble_ledsaver_inst;

/** @brief module init params. This module requires 2 flash pages*/
typedef struct 
{
    uint32_t                            first_page;   
    uint32_t                            second_page;
    ble_ledstatesaver_state_changed     state_changed_callback;
    void*                               p_data;     /* User-defined parameter passed to state changed callback */
}ble_ledsaver_init;

/**
 * @brief init module
 *
 * @param p_init_params     pointer to module instance struct
 * @param p_saver_inst      pointer to init param struct
 * @return true if OK, false if Error
 */
bool led_state_saver_init(ble_ledsaver_inst* p_saver_inst, const ble_ledsaver_init* p_init_params);

/**
 * @brief Get led state from flash
 *
 * @param p_saver_inst      pointer to module instance struct
 * @param p_hsv_led         pointer to led state
 * @return true if OK, false if Error
 */
bool led_state_saver_get_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led);

/**
 * @brief save led state to flash
 *
 * @param p_saver_inst      pointer to module instance struct
 * @param p_hsv_led         pointer to led state
 * @return true if OK, false if Error
 */
bool led_state_saver_save_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led);

#endif