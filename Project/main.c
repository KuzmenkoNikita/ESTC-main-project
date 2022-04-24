
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

#include "nrfx_timer.h"

#define TIMER_PERIOD_MS     500

static ble_communicator_t ble_communicator;
static nrfx_timer_t m_timer = NRFX_TIMER_INSTANCE(3);                            

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}
/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */

static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

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

    SRGBCoordinates sRGB;
    HSVtoRGB_calc(p_hsv_color, &sRGB);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);
}


void main_tmr_callback(nrf_timer_event_t event_type, void* p_context)
{
    SHSVCoordinates* psLedCoords = (SHSVCoordinates*)p_context;

    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
        {
            if(psLedCoords->S == 100)
            {
                psLedCoords->S = 0;
            }
            else
            {
                ++psLedCoords->S;
            }

            if(psLedCoords->V == 0)
            {
                psLedCoords->V = 100;
            }
            else
            {
                --psLedCoords->V;
            }

            SRGBCoordinates sRGB;
            HSVtoRGB_calc(psLedCoords, &sRGB);

            pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

            break;
        }

        default:
        {
            break;
        }
    }
}

int main(void)
{
    log_init();
    timers_init();

    SHSVCoordinates sLedCoords;
    nrfx_timer_config_t sTmrCfg = NRFX_TIMER_DEFAULT_CONFIG;
    sLedCoords.V = 100;
    sLedCoords.S = 100;
    sLedCoords.H = 360;

    sTmrCfg.frequency = NRF_TIMER_FREQ_1MHz;
    sTmrCfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    sTmrCfg.p_context = (void*)&sLedCoords;

    nrfx_timer_init(&m_timer, &sTmrCfg, main_tmr_callback);
    uint32_t unTicks = nrfx_timer_ms_to_ticks(&m_timer, TIMER_PERIOD_MS);
    nrfx_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, unTicks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrfx_timer_enable(&m_timer);

    
    pca10059_RGBLed_init();

    ble_comm_init_t ble_comm_init;

    ble_comm_init.p_ctx             = (void*)&sLedCoords;
    ble_comm_init.led_set_color_cb  = ble_set_led_color_component;

    ble_communicaror_init(&ble_communicator, &ble_comm_init);

    while(1)
    {
        ble_communicator_notify_color(&ble_communicator, BLE_LED_COMPONENT_V, sLedCoords.V);
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
    
}


/**
 * @}
 */
