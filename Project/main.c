
/** @file
 *
 * @defgroup estc_adverts main.c
 * @{
 * @ingroup estc_templates
 * @brief ESTC Advertisments template app.
 *
 * This file contains a template for creating a new BLE application with GATT services. It has
 * the code necessary to advertise, get a connection, restart advertising on disconnect.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "app_error.h"

#include "app_timer.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "ble_communicator.h"
#include "pca10059_rgb_led.h"
#include "HSV_to_RGB_Calc.h"

#include "BLE_LedStateSaver.h"

static ble_communicator_t ble_communicator;
            static bool is_connected = false;               
/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/** @brief Function for the Timer initialization.
 *
 *  @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/** @brief Callback fuction for changing LED color
 *
 */
void ble_set_led_color_component(ble_led_components color_component, uint16_t value, void* p_ctx)
{
    SHSVCoordinates* p_hsv_color = (SHSVCoordinates*)p_ctx;

    switch(color_component)
    {
        case BLE_LED_COMPONENT_H:
        {
            p_hsv_color->H = value;
            break;
        }

        case BLE_LED_COMPONENT_S:
        {
            p_hsv_color->S = value;
            break;
        }

        case BLE_LED_COMPONENT_V:
        {
            p_hsv_color->V = value;
            break;
        }

        default: return;
    }
is_connected = true;
    SRGBCoordinates sRGB;
    HSVtoRGB_calc(p_hsv_color, &sRGB);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);
}

void ble_ledsaver_state_changed(ble_ledsaver_state state, void* p_data)
{
    switch(state)
    {
        case BLE_LEDSAVER_INIT_SUCCESSFUL:
        {
            NRF_LOG_INFO("BLE_LEDSAVER_INIT_SUCCESSFUL");
            ble_ledsaver_inst* p_ledsaver_inst = (ble_ledsaver_inst*)p_data;
            ;
            SHSVCoordinates led_hsv;

            led_state_saver_get_state(p_ledsaver_inst, &led_hsv);

            break;
        }

        case BLE_LEDSAVER_INIT_ERROR:
        {
            NRF_LOG_INFO("BLE_LEDSAVER_INIT_ERROR");
            break;
        }

        case BLE_LEDSAVER_WRITE_SUCCESSFUL:
        {
            NRF_LOG_INFO("BLE_LEDSAVER_WRITE_SUCCESSFUL");
            break;
        }

        default:
        {
            NRF_LOG_INFO("ble_ledsaver_state_changed: unexpected state");
            break;
        }
    }
}

int main(void)
{
    log_init();
    timers_init();

    SHSVCoordinates sLedCoords = {0};

    pca10059_RGBLed_init();

    ble_comm_init_t ble_comm_init;

    ble_comm_init.p_ctx             = (void*)&sLedCoords;
    ble_comm_init.led_set_color_cb  = ble_set_led_color_component;

    ble_communicaror_init(&ble_communicator, &ble_comm_init);

    ble_ledsaver_inst ledsaver_inst;
    ble_ledsaver_init ble_ledsaver_init;

    ble_ledsaver_init.first_page    = 0x000DE000;
    ble_ledsaver_init.second_page   = 0x000DD000;
    ble_ledsaver_init.p_data        = (void*)&ledsaver_inst;
    ble_ledsaver_init.state_changed_callback = ble_ledsaver_state_changed;

    

    while(1)
    {
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();

    if(is_connected)
    {
        is_connected = false;
        led_state_saver_init(&ledsaver_inst, &ble_ledsaver_init);
    }

        

    

    }
    
}
