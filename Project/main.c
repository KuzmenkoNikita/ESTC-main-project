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
#include "WMIndication.h"
#include "pca10059_button.h"
/* ******************************************************************* */
#define WAIT_APPLY_PRESSING_MS      200
/* ******************************************************************* */
static ble_ledsaver_inst ledsaver_inst;
static ble_communicator_t ble_communicator;
static ble_ledsaver_state   ledsaver_init_state = BLE_LEDSAVER_STATE_UNDEFINED;
static ble_ledsaver_state   ledsaver_save_state = BLE_LEDSAVER_WRITE_SUCCESSFUL;
static SHSVCoordinates hsv_led_coords = {0};
static EWMTypes current_workmode = EWM_NO_INPUT;
static uint32_t global_time = 0;
static eBtnState main_button_state = BTN_UNDEFINED;

APP_TIMER_DEF(global_time_timer);
/* ******************************************************************* */
void global_time_tiomeout_handler(void * p_context);
static void increment_hsv(SHSVCoordinates* psHSV, EWMTypes eWM);
static void send_hsv_depending_on_wm(SHSVCoordinates* psHSV, EWMTypes eWM);
static void save_led_hsv_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led);
/* ******************************************************************* */
/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}
/* ******************************************************************* */
static void save_led_hsv_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led)
{
    if(!p_saver_inst || !p_hsv_led)
        return;

    if(ledsaver_save_state != BLE_LEDSAVER_STATE_UNDEFINED)
    {
        ledsaver_save_state = BLE_LEDSAVER_STATE_UNDEFINED;

        if(!led_state_saver_save_state(p_saver_inst, p_hsv_led))
        {
            NRF_LOG_INFO("ble_set_led_color_component: led_state_saver_save_state error");
        }
    }
    else
    {
        NRF_LOG_INFO("ble_set_led_color_component: can't save led state - flash is not ready");
    }   
}
/* ******************************************************************* */
static void send_hsv_depending_on_wm(SHSVCoordinates* psHSV, EWMTypes eWM)
{
    if(!psHSV)
        return;

    uint16_t color_value = 0;
    ble_led_components led_color;

    switch(eWM)
    {
        case EWM_TUNING_H:
        {
            led_color = BLE_LED_COMPONENT_H;
            color_value = psHSV->H;
            break;
        }

        case EWM_TUNING_S:
        {
            led_color = BLE_LED_COMPONENT_S;
            color_value = psHSV->S;
            break;
        }

        case EWM_TUNING_V:
        {
            led_color = BLE_LED_COMPONENT_V;
            color_value = psHSV->V;
            break;
        }

        default: return;
    }

    if(!ble_communicator_send_color(&ble_communicator, led_color, color_value))
    {
        NRF_LOG_INFO("send_hsv_depending_on_wm: ble_communicator_send_color error!");
    }
}
/* ******************************************************************* */
static void increment_hsv(SHSVCoordinates* psHSV, EWMTypes eWM)
{
    EHSVParams eHSVParam;
    if(!psHSV)
        return;

    switch(eWM)
    {
        case EWM_TUNING_H:
        {
            eHSVParam = E_PARAM_H;
            break;
        }

        case EWM_TUNING_S:
        {
            eHSVParam = E_PARAM_S;
            break;
        }

        case EWM_TUNING_V:
        {
            eHSVParam = E_PARAM_V;
            break;
        }

        default: return;
    }

    increment_with_rotate(psHSV, eHSVParam);
}
/* ************************************************************************ */
void global_time_tiomeout_handler(void * p_context)
{
    if(main_button_state == BTN_RELEASED)
    {
        global_time = 0;
    }
    else if (main_button_state == BTN_PRESSED)
    {
        ++global_time;

        if(global_time == WAIT_APPLY_PRESSING_MS)
        {
            global_time = 0;

            increment_hsv(&hsv_led_coords, current_workmode);

            SRGBCoordinates sRGB;
            HSVtoRGB_calc(&hsv_led_coords, &sRGB);
            pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

            send_hsv_depending_on_wm(&hsv_led_coords, current_workmode);

            NRF_LOG_INFO("Setting H %u , S %u , V %u \n", hsv_led_coords.H, hsv_led_coords.S, hsv_led_coords.V);
        }
    }
    else
    {
        return;
    }

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

    err_code = app_timer_create(&global_time_timer, APP_TIMER_MODE_REPEATED, global_time_tiomeout_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(global_time_timer, APP_TIMER_TICKS(1), NULL);
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

    SRGBCoordinates sRGB;
    HSVtoRGB_calc(p_hsv_color, &sRGB);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

    save_led_hsv_state(&ledsaver_inst, p_hsv_color);
}
/* ********************************************************************** */
void ble_ledsaver_state_changed(ble_ledsaver_state state, void* p_data)
{
    switch(state)
    {
        case BLE_LEDSAVER_INIT_SUCCESSFUL:
        {
            NRF_LOG_INFO("BLE_LEDSAVER_INIT_SUCCESSFUL");
            ledsaver_init_state = BLE_LEDSAVER_INIT_SUCCESSFUL;

            break;
        }

        case BLE_LEDSAVER_INIT_ERROR:
        {
            NRF_LOG_INFO("BLE_LEDSAVER_INIT_ERROR");
            ledsaver_init_state = BLE_LEDSAVER_INIT_ERROR;
            break;
        }

        case BLE_LEDSAVER_WRITE_SUCCESSFUL:
        {
            ledsaver_save_state = BLE_LEDSAVER_WRITE_SUCCESSFUL;
            NRF_LOG_INFO("BLE_LEDSAVER_WRITE_SUCCESSFUL");
            
            break;
        }

        case BLE_LEDSAVER_WRITE_ERROR:
        {
            ledsaver_save_state = BLE_LEDSAVER_WRITE_ERROR;
            NRF_LOG_INFO("BLE_LEDSAVER_WRITE_ERROR");
            break;
        }

        default:
        {
            NRF_LOG_INFO("ble_ledsaver_state_changed: unexpected state");
            break;
        }
    }
}
/* ********************************************************************** */
void button_dblclick_handler(eBtnState eState, void* pData)
{
    NRF_LOG_INFO("Workmode changed");

    if(current_workmode == EWM_COUNT_WM - 1)
    {
        current_workmode = EWM_NO_INPUT;

        save_led_hsv_state(&ledsaver_inst, &hsv_led_coords);
    }
    else
    {
        ++current_workmode;
    }

    WMIndication_SetWM(current_workmode);
}
/* ********************************************************************** */
int main(void)
{
    log_init();
    timers_init();

    pca10059_RGBLed_init();

    ble_comm_init_t ble_comm_init;

    if(0 != WMIndication_init())
    {
        NRF_LOG_INFO("WMIndication_init failed!");
    }

    WMIndication_SetWM(current_workmode);

    ble_comm_init.p_ctx             = (void*)&hsv_led_coords;
    ble_comm_init.led_set_color_cb  = ble_set_led_color_component;

    if(!ble_communicaror_init(&ble_communicator, &ble_comm_init))
    {
        NRF_LOG_INFO("ble_communicaror_init failed!");
    }

    SBtnIRQParams button_init_params;

    button_init_params.eBtnIrqState = BTN_DOUBLE_CLICKED;
    button_init_params.fnBtnHandler = button_dblclick_handler;
    button_init_params.pUserData    = (void*)&current_workmode;
    button_init_params.unHiTmrNum   = 1;
    button_init_params.unLowTmrNum  = 2;

    pca10059_button_init(&button_init_params);
    pca10059_button_enable_irq();

    ble_ledsaver_init ble_ledsaver_init;

    ble_ledsaver_init.first_page    = 0x000DE000;
    ble_ledsaver_init.second_page   = 0x000DD000;
    ble_ledsaver_init.p_data        = (void*)&ledsaver_inst;
    ble_ledsaver_init.state_changed_callback = ble_ledsaver_state_changed;

    if(!led_state_saver_init(&ledsaver_inst, &ble_ledsaver_init))
    {
        NRF_LOG_INFO("led_state_saver_init failed!");
    }

    while(ledsaver_init_state == BLE_LEDSAVER_STATE_UNDEFINED);

    if(ledsaver_init_state != BLE_LEDSAVER_INIT_SUCCESSFUL)
    {
        NRF_LOG_INFO("led_state_saver_init error: no status changed");
    }

    if(!led_state_saver_get_state(&ledsaver_inst, &hsv_led_coords))
    {
        NRF_LOG_INFO("led_state_saver_get_state error");
    }

    SRGBCoordinates sRGB;
    HSVtoRGB_calc(&hsv_led_coords, &sRGB);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);
    

    while(1)
    {
        main_button_state = pca10059_GetButtonState();

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}
