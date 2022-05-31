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
#include "BLE_LedStateSaver.h"
#include "WMIndication.h"
#include "pca10059_button.h"
#include "nrf_ringbuf.h"
/* ******************************************************************* */
#define WAIT_APPLY_PRESSING_MS      200
#define MAX_CMD_BUF_SIZE            200
#define SEND_H_CODE                 1
#define SEND_S_CODE                 2
#define SEND_V_CODE                 3
/* ******************************************************************* */
static ble_ledsaver_inst ledsaver_inst;
static ble_communicator_t ble_communicator;
static ble_ledsaver_state   ledsaver_init_state = BLE_LEDSAVER_STATE_UNDEFINED;
static ble_ledsaver_state   ledsaver_save_state = BLE_LEDSAVER_WRITE_SUCCESSFUL;
static SHSVCoordinates hsv_led_coords = {0};
static EWMTypes current_workmode = EWM_NO_INPUT;
static uint32_t global_time = 0;
static eBtnState main_button_state = BTN_UNDEFINED;
static uint32_t bytes_in_ringbuf = 0;
static bool is_acksend_complete = true;
/* ******************************************************************* */
typedef struct 
{
    uint32_t color_value;
    uint32_t color_code;
}ack_send_data;
/* ******************************************************************* */

APP_TIMER_DEF(global_time_timer);
NRF_RINGBUF_DEF(m_ringbuf, sizeof(ack_send_data)*32);
/* ******************************************************************* */
void global_time_tiomeout_handler(void * p_context);
static void increment_hsv(SHSVCoordinates* psHSV, EWMTypes eWM);
static void send_hsv_depending_on_wm(SHSVCoordinates* psHSV, EWMTypes eWM);
static void save_led_hsv_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led);
void parser_help_request_callback(void* p_data, const char** p_m_sz_info, uint32_t  array_size);
void parser_cmd_set_rgb_callback(uint8_t r, uint8_t g, uint8_t b, void* p_data);
void parser_cmd_set_hsv_callback(uint16_t h, uint8_t s, uint8_t v, void* p_data);
void parser_cmd_error_callback(void* p_data);
static void send_all_color_components(SHSVCoordinates* psHSV);
void ble_ack_send_done_callback(void* p_ctx);
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

    if(!ble_communicator_send_color(&ble_communicator, led_color, color_value, false))
    {
        NRF_LOG_INFO("send_hsv_depending_on_wm: ble_communicator_send_color error!");
    }
}
/* ******************************************************************* */
static void send_all_color_components(SHSVCoordinates* psHSV)
{
    if(!psHSV)
        return;

    ack_send_data ack_send_info[3] = {{psHSV->H, SEND_H_CODE}, 
                                    {psHSV->S, SEND_S_CODE},
                                    {psHSV->V, SEND_V_CODE}};

    for(uint32_t i = 0; i < 3; ++i)
    {

        size_t allocated_size = sizeof(ack_send_data);

        if(NRF_SUCCESS != nrf_ringbuf_cpy_put(&m_ringbuf, (uint8_t*)&ack_send_info[i], &allocated_size))
        {
            NRF_LOG_INFO("send_all_color_components: nrf_ringbuf_cpy_put error!");
        }
        else
        {
            bytes_in_ringbuf+= allocated_size;
        }
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

    save_led_hsv_state(&ledsaver_inst, &hsv_led_coords);
    send_all_color_components(&hsv_led_coords);

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

    save_led_hsv_state(&ledsaver_inst, &hsv_led_coords);
    send_all_color_components(&hsv_led_coords);

    sprintf(sz_msg, "Color is set to H: %u, S: %u, V: %u \n\r", h,s,v);

    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    }
    
}
/* ********************************************************************** */
void ble_ack_send_done_callback(void* p_ctx)
{
    is_acksend_complete = true;
    NRF_LOG_INFO("ble_ack_send_done_callback ");
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
    nrf_ringbuf_init(&m_ringbuf);

    ble_comm_init.p_ctx             = (void*)&hsv_led_coords;
    ble_comm_init.led_set_color_cb  = ble_set_led_color_component;
    ble_comm_init.send_ack_callback = ble_ack_send_done_callback;

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

    SLEDStateParserInst parser_inst;
    SLEDStateParserInfo parser_info;

    parser_info.p_data          = NULL;
    parser_info.help_request    = parser_help_request_callback;
    parser_info.cmd_error       = parser_cmd_error_callback;
    parser_info.set_rgb         = parser_cmd_set_rgb_callback;
    parser_info.set_hsv         = parser_cmd_set_hsv_callback;

    parser_init(&parser_inst, &parser_info);
    usb_agent_init();

    uint32_t ack_send_data_size = sizeof(ack_send_data);
    ack_send_data ack_data_to_send;

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

        if(is_acksend_complete)
        {
            if(bytes_in_ringbuf >= ack_send_data_size)
            {

                NRF_LOG_INFO("Bytes in ringbuf %u send_data_size %u", bytes_in_ringbuf, ack_send_data_size);

                size_t get_len = ack_send_data_size;

               if(NRF_SUCCESS != nrf_ringbuf_cpy_get(&m_ringbuf, (uint8_t*)&ack_data_to_send, &get_len))
               {
                   NRF_LOG_INFO("nrf_ringbuf_cpy_get acksend error");
               }
               else
               {
                    bytes_in_ringbuf-=get_len;
                    
                    ble_led_components send_led_color;

                    switch(ack_data_to_send.color_code)
                    {
                        case SEND_H_CODE:
                        {
                            send_led_color = BLE_LED_COMPONENT_H;
                            break;
                        }

                        case SEND_S_CODE:
                        {
                            send_led_color = BLE_LED_COMPONENT_S;
                            break;
                        }

                        case SEND_V_CODE:
                        {
                            send_led_color = BLE_LED_COMPONENT_V;
                            break;
                        }

                        default:
                        {
                            send_led_color = BLE_LED_COMPONENT_H;
                            break;
                        } 
                    }
                    
                    is_acksend_complete = false;
                    if(!ble_communicator_send_color(&ble_communicator, send_led_color, ack_data_to_send.color_value, true))
                    {
                        is_acksend_complete = true;
                        NRF_LOG_INFO("ble_communicator_send_color acksend error");
                    }
               }

            }
        }

        main_button_state = pca10059_GetButtonState();

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}
