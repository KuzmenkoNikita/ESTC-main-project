#ifndef FDS_LEDSTATESAVER_H
#define FDS_LEDSTATESAVER_H

#include <stdint.h>
#include <stdbool.h>
#include "HSV_to_RGB_Calc.h"



/** @brief module state */
typedef enum
{
    FDS_LEDSAVER_STATE_UNDEFINED,
    FDS_LEDSAVER_INIT_COMPLETE,
    FDS_LEDSAVER_INIT_ERROR,
    FDS_LEDSAVER_WRITE_COMPLETE,
    FDS_LEDSAVER_WRITE_ERROR
}fds_ledsaver_state;

/**
 * @brief Button interrupt handler
 *
 * @param eState   button state
 * @param pData pointer to user data
 */
typedef void (*fds_ledsaver_state_changed)(fds_ledsaver_state state, void* p_data);

/** @brief module init params */
typedef struct 
{
    fds_ledsaver_state_changed  fn_state_changed;
    void*                       p_data;
}fds_ledsaver_init;

/**
 * @brief Module initialization
 *
 * @param p_init_params pointer to module initialization params
 */
bool fds_led_state_saver_init(const fds_ledsaver_init* p_init_params);

/**
 * @brief Retrieve led color state from flash
 *
 * @param p_hsv_led pointer to led color state
 */
bool fds_led_state_saver_get_state(SHSVCoordinates* p_hsv_led);

/**
 * @brief Save led color state to flash
 *
 * @param p_hsv_led pointer to led color state
 */
bool fds_led_state_saver_save_state(const SHSVCoordinates* p_hsv_led);


#endif /* FDS_LEDSTATESAVER_H */