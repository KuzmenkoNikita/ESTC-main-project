#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "app_error.h"
#include "LEDState_Parser.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "usb_agent.h"
#include "ble_communicator.h"
#include "pca10059_rgb_led.h"
#include "HSV_to_RGB_Calc.h"
#include "WMIndication.h"
#include "pca10059_button.h"
#include "FDS_LedStateSaver.h"

/* ******************************************************************* */
#define WAIT_APPLY_PRESSING_MS      200
#define MAX_CMD_BUF_SIZE            200
#define SEND_H_CODE                 1
#define SEND_S_CODE                 2
#define SEND_V_CODE                 3
/* ******************************************************************* */
static ble_communicator_t ble_communicator;
static fds_ledsaver_state   ledsaver_init_state = FDS_LEDSAVER_STATE_UNDEFINED;
static fds_ledsaver_state   ledsaver_save_state = FDS_LEDSAVER_WRITE_COMPLETE;
static SHSVCoordinates hsv_led_coords = {0};
static EWMTypes current_workmode = EWM_NO_INPUT;
static uint32_t global_time = 0;
static eBtnState main_button_state = BTN_UNDEFINED;
/* ******************************************************************* */
typedef struct 
{
    uint32_t color_value;
    uint32_t color_code;
}ack_send_data;
/* ******************************************************************* */

APP_TIMER_DEF(global_time_timer);
/* ******************************************************************* */
void global_time_tiomeout_handler(void * p_context);
static void increment_hsv(SHSVCoordinates* psHSV, EWMTypes eWM);
static void save_led_hsv_state(/* ble_ledsaver_inst* p_saver_inst,*/ SHSVCoordinates* p_hsv_led);
void parser_help_request_callback(void* p_data, const char** p_m_sz_info, uint32_t  array_size);
void parser_cmd_set_rgb_callback(uint8_t r, uint8_t g, uint8_t b, void* p_data);
void parser_cmd_set_hsv_callback(uint16_t h, uint8_t s, uint8_t v, void* p_data);
void parser_cmd_error_callback(void* p_data);
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
void fds_ledstatesaver_callback(fds_ledsaver_state state, void* p_data)
{
    switch(state)
    {
        case FDS_LEDSAVER_INIT_COMPLETE:
        {
            ledsaver_init_state = FDS_LEDSAVER_INIT_COMPLETE;
            break;
        }

        case FDS_LEDSAVER_INIT_ERROR:
        {
            ledsaver_init_state = FDS_LEDSAVER_INIT_ERROR;
            break;
        }

        case FDS_LEDSAVER_WRITE_COMPLETE:
        {
            ledsaver_save_state = FDS_LEDSAVER_WRITE_COMPLETE;
            break;
        }

        case FDS_LEDSAVER_WRITE_ERROR:
        {
            ledsaver_save_state = FDS_LEDSAVER_WRITE_ERROR;
            break;
        }

        default:
        {
            return;
        }
    }
}
/* ******************************************************************* */
static void save_led_hsv_state(/* ble_ledsaver_inst* p_saver_inst, */SHSVCoordinates* p_hsv_led)
{
    if(!p_hsv_led)
        return;

    if(ledsaver_save_state != FDS_LEDSAVER_STATE_UNDEFINED)
    {
        ledsaver_save_state = FDS_LEDSAVER_STATE_UNDEFINED;

        if(!fds_led_state_saver_save_state(p_hsv_led))
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

            if(!ble_communicator_send_color(&ble_communicator, sRGB.R, sRGB.G, sRGB.B))
            {
                NRF_LOG_INFO("global_time_tiomeout_handler: ble_communicator_send_color error ");
            }

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
void ble_set_led_color_component(uint8_t R, uint8_t G, uint8_t B, void* p_ctx)
{
    SRGBCoordinates sRGB = {R, G, B};
    SHSVCoordinates sHSV;

    RGBtoHSV_calc(&sRGB, &sHSV);

    hsv_led_coords = sHSV;
   
    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

    NRF_LOG_INFO("BLE led set: R: %u, G: %u, B: %u", sRGB.R, sRGB.G, sRGB.B);

    save_led_hsv_state(/* &ledsaver_inst,*/ &hsv_led_coords);

    if(!ble_communicator_send_color(&ble_communicator, sRGB.R, sRGB.G, sRGB.B))
    {
        NRF_LOG_INFO("ble_set_led_color_component: ble_communicator_send_color error ");
    }
}
/* ********************************************************************** */
void button_dblclick_handler(eBtnState eState, void* pData)
{
    NRF_LOG_INFO("Workmode changed");

    if(current_workmode == EWM_COUNT_WM - 1)
    {
        current_workmode = EWM_NO_INPUT;

        save_led_hsv_state(/*&ledsaver_inst,*/ &hsv_led_coords);
    }
    else
    {
        ++current_workmode;
    }

    WMIndication_SetWM(current_workmode);
}
/* ********************************************************************** */
void parser_help_request_callback(void* p_data, const char** p_m_sz_info, uint32_t  array_size)
{
    for(uint32_t i = 0; i < array_size; ++i)
    {
        if(!usb_agent_send_buf(p_m_sz_info[i], strlen(p_m_sz_info[i])))
        {
            NRF_LOG_INFO("Error send help msg \n");
        }
    }
}
/* ********************************************************************** */
void parser_cmd_error_callback(void* p_data)
{
    const char sz_msg[]="Incorrect command! Try \"help\" for info\n\r";

    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    } 
}
/* ********************************************************************** */
void parser_cmd_set_rgb_callback(uint8_t r, uint8_t g, uint8_t b, void* p_data)
{
    char sz_msg[50];
    SRGBCoordinates sRGB = {r, g, b};
    SHSVCoordinates sHSV;

    RGBtoHSV_calc(&sRGB, &sHSV);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

    hsv_led_coords = sHSV;

    save_led_hsv_state(/*&ledsaver_inst,*/ &hsv_led_coords);
    
    if(!ble_communicator_send_color(&ble_communicator, sRGB.R, sRGB.G, sRGB.B))
    {
        NRF_LOG_INFO("parser_cmd_set_rgb_callback: ble_communicator_send_color error ");
    }

    sprintf(sz_msg, "Color is set to R: %u, G: %u, B: %u \n\r", r,g,b);

    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    }
}
/* ********************************************************************** */
void parser_cmd_set_hsv_callback(uint16_t h, uint8_t s, uint8_t v, void* p_data)
{
    
    char sz_msg[50];
    SRGBCoordinates sRGB;
    SHSVCoordinates sHSV = {h, s, v};

    HSVtoRGB_calc(&sHSV, &sRGB);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

    hsv_led_coords = sHSV;

    save_led_hsv_state(/*&ledsaver_inst,*/ &hsv_led_coords);
    
    if(!ble_communicator_send_color(&ble_communicator, sRGB.R, sRGB.G, sRGB.B))
    {
        NRF_LOG_INFO("parser_cmd_set_rgb_callback: ble_communicator_send_color error ");
    }

    sprintf(sz_msg, "Color is set to H: %u, S: %u, V: %u \n\r", h,s,v);

    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    }
    
}
/* ********************************************************************** */
 void ble_client_connected_callback(void* p_ctx)
 {
    SRGBCoordinates sRGB;
    RGBtoHSV_calc(&sRGB, &hsv_led_coords);

    if(!ble_communicator_send_color(&ble_communicator, sRGB.R, sRGB.G, sRGB.B))
    {
        NRF_LOG_INFO("ble_client_connected_callback: ble_communicator_send_color error ");
    }

    NRF_LOG_INFO("ble_client_connected_callback");
 }

 /* ********************************************************************** */
 void ble_client_disconnected_callback(void* p_ctx)
 {
     NRF_LOG_INFO("ble_client_disconnected_callback");
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

    fds_ledsaver_init ledsaver_init_params;
    ledsaver_init_params.p_data             = 0;
    ledsaver_init_params.fn_state_changed   = fds_ledstatesaver_callback;   

    WMIndication_SetWM(current_workmode);

    ble_comm_init.p_ctx                     = (void*)&hsv_led_coords;
    ble_comm_init.led_set_color_cb          = ble_set_led_color_component;
    ble_comm_init.client_connected_cb       = ble_client_connected_callback;
    ble_comm_init.client_disconnected_cb    = ble_client_disconnected_callback;

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
    
    if(!fds_led_state_saver_init(&ledsaver_init_params))
    {
        NRF_LOG_INFO("fds_led_state_saver_init ERROR!");
    }

    while(ledsaver_init_state == FDS_LEDSAVER_STATE_UNDEFINED)
    {
        NRF_LOG_INFO("waiting for led_state_saver_init...");
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }

    if(ledsaver_init_state != FDS_LEDSAVER_INIT_COMPLETE)
    {
        NRF_LOG_INFO("fds_led_state_saver_init: BAD init flag");
    }


    if(!fds_led_state_saver_get_state(&hsv_led_coords))
    {
        NRF_LOG_INFO("fds_led_state_saver_get_state ERROR!");
    }


    SRGBCoordinates sRGB;
    HSVtoRGB_calc(&hsv_led_coords, &sRGB);

    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

    SLEDStateParserInst parser_inst;
    SLEDStateParserInfo parser_info;

    parser_info.p_data          = NULL;
    parser_info.help_request    = parser_help_request_callback;
    parser_info.cmd_error       = parser_cmd_error_callback;
    parser_info.set_rgb         = parser_cmd_set_rgb_callback;
    parser_info.set_hsv         = parser_cmd_set_hsv_callback;

    parser_init(&parser_inst, &parser_info);
    usb_agent_init();

    while(1)
    {
        size_t unCMDSize = 0;

        if(usb_agent_process(&unCMDSize))
        {
            if(unCMDSize > MAX_CMD_BUF_SIZE)
            {
                usb_agent_reset_cmd_buf();
            }
            else
            {
                char p_cmd[unCMDSize + 1];
                if(usb_agent_get_cmd_buf(p_cmd, unCMDSize + 1))
                {
                    p_cmd[unCMDSize] = '\0';
                    parser_parse_cmd(&parser_inst, p_cmd);
                }
            }
        }

        main_button_state = pca10059_GetButtonState();

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}
