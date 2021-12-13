#include <stdbool.h>
#include <stdint.h>
#include "pca10059_button.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "pca10059_LedsBlinkByParams.h"

#include "nrf_delay.h"
#include "WMIndication.h"

#include "nrf_gpio.h"
#include "pca10059_rgb_led.h"
#include "HSV_to_RGB_Calc.h"
#include "LedStateSaver.h"
#include "LEDState_Parser.h"
#include "usb_agent.h"

#define WAIT_AFTER_CHANGE_WM_MS     400
#define WAIT_APPLY_PARAM_MS         200

#define LEDSTATESAVER_FIRST_PAGE_ADDR   0x000DD000
#define LEDSTATESAVER_SECOND_PAGE_ADDR  0x000DE000

#define MAX_CMD_BUF_SIZE                200

typedef struct 
{
    SHSVCoordinates*    psHSV;
    EWMTypes*           peCurrentWM;
    SLedStateSaverInst* psSaver;
}SButtonIRQData;


/**
 * @brief Init logs
 */
void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**
 * @brief Button Double-click handler
 */
void ButtonHandler(eBtnState eState, void* pData)
{
    if(!pData)
        return;

    SButtonIRQData* sBtnIRQData =  (SButtonIRQData*)pData;

    if(*(sBtnIRQData->peCurrentWM) == EWM_COUNT_WM - 1)
    {
        *(sBtnIRQData->peCurrentWM)  = EWM_NO_INPUT;
        SLEDColorState sLedColorState;

        sLedColorState.H    = sBtnIRQData->psHSV->H;
        sLedColorState.S    = sBtnIRQData->psHSV->S;
        sLedColorState.V    = sBtnIRQData->psHSV->V;

        LedStateSaver_SaveLedState(sBtnIRQData->psSaver, &sLedColorState);
    }
    else
        *(sBtnIRQData->peCurrentWM)= *(sBtnIRQData->peCurrentWM) + 1;

    WMIndication_SetWM(*(sBtnIRQData->peCurrentWM));
}
/* ******************************************************************* */
void IncrementHSVByWormode(SHSVCoordinates* psHSV, EWMTypes eWM)
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
/* *********************************************** */
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
/* *********************************************** */
void parser_cmd_error_callback(void* p_data)
{
    const char sz_msg[]="Incorrect command! Try \"help\" for info\n\r";

    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    } 
}
/* *********************************************** */
void parser_cmd_set_rgb_callback(uint8_t r, uint8_t g, uint8_t b, void* p_data)
{
    char sz_msg[50];
    SRGBCoordinates sRGB;
    SButtonIRQData* ps_iqr_str = (SButtonIRQData*)p_data;
    if(!ps_iqr_str)
        return;

    sprintf(sz_msg, "Color is set to R: %u, G: %u, B: %u \n\r", r,g,b);

    pca10059_RGBLed_Set(r, g, b);

    sRGB.R = r;
    sRGB.G = g;
    sRGB.B = b;

    RGBtoHSV_calc(&sRGB, ps_iqr_str->psHSV);

    SLEDColorState sLedColorState;

    sLedColorState.H    = ps_iqr_str->psHSV->H;
    sLedColorState.S    = ps_iqr_str->psHSV->S;
    sLedColorState.V    = ps_iqr_str->psHSV->V;

    LedStateSaver_SaveLedState(ps_iqr_str->psSaver, &sLedColorState);


    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    }
}
/* *********************************************** */
void parser_cmd_set_hsv_callback(uint16_t h, uint8_t s, uint8_t v, void* p_data)
{
    char sz_msg[50];
    SRGBCoordinates sRGB;
    SButtonIRQData* ps_iqr_str = (SButtonIRQData*)p_data;
    if(!ps_iqr_str)
        return;

    sprintf(sz_msg, "Color is set to H: %u, S: %u, V: %u \n\r", h,s,v);

    ps_iqr_str->psHSV->H    = h;
    ps_iqr_str->psHSV->S    = s;
    ps_iqr_str->psHSV->V    = v;

    HSVtoRGB_calc(ps_iqr_str->psHSV, &sRGB);
    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);


    SLEDColorState sLedColorState;

    sLedColorState.H    = ps_iqr_str->psHSV->H;
    sLedColorState.S    = ps_iqr_str->psHSV->S;
    sLedColorState.V    = ps_iqr_str->psHSV->V;

    LedStateSaver_SaveLedState(ps_iqr_str->psSaver, &sLedColorState);

    if(!usb_agent_send_buf(sz_msg, strlen(sz_msg)))
    {
        NRF_LOG_INFO("Error send set RGB msg \n");
    }
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    SBtnIRQParams sBtnIrq;
    EWMTypes eCurrentWM = EWM_NO_INPUT;
    EWMTypes eNewWM = EWM_NO_INPUT;

    SHSVCoordinates sHSV;
    SRGBCoordinates sRGB;
    SLedStateSaverInst sLedStateSaver;
    SLedStateSaverParam sLedStateSaverParam;

    sLedStateSaverParam.unFirstPageAddr = LEDSTATESAVER_FIRST_PAGE_ADDR;
    sLedStateSaverParam.unSecondPageAddr = LEDSTATESAVER_SECOND_PAGE_ADDR;
    LedStateSaver_init(&sLedStateSaver, &sLedStateSaverParam);
    SLEDColorState sledState;
    SButtonIRQData sBtnIRQData;
    sBtnIRQData.psHSV = &sHSV;
    sBtnIRQData.peCurrentWM = &eCurrentWM;
    sBtnIRQData.psSaver = &sLedStateSaver;

    sHSV.H = 0;
    sHSV.S = 100;
    sHSV.V = 100;

    if(0 == LedStateSaver_GetStateFromFlash(&sLedStateSaver, &sledState))
    {
        
        sHSV.H = sledState.H;
        sHSV.S = sledState.S;
        sHSV.V = sledState.V;
    }

    HSVtoRGB_calc(&sHSV, &sRGB);

    sBtnIrq.eBtnIrqState    = BTN_DOUBLE_CLICKED;
    sBtnIrq.fnBtnHandler    = ButtonHandler;
    sBtnIrq.pUserData       = (void*)&sBtnIRQData;
    sBtnIrq.unHiTmrNum      = 0;
    sBtnIrq.unLowTmrNum     = 1;
    
    logs_init();

    pca10059_RGBLed_init();
    pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

    WMIndication_init();

    WMIndication_SetWM(eCurrentWM);

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    uint32_t unTimeCnt = 0;
    uint32_t unWaitWMTimeout = 0;

    SLEDStateParserInst parser_inst;
    SLEDStateParserInfo parser_info;

    parser_info.p_data          = (void*)&sBtnIRQData;
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

        /* Small delay after changing WM */
        if(eNewWM != eCurrentWM)
        {
            nrf_delay_ms(10);
            unWaitWMTimeout+=10;
            if(unWaitWMTimeout == WAIT_AFTER_CHANGE_WM_MS)
            {
                unWaitWMTimeout = 0;
                eNewWM = eCurrentWM;
                NRF_LOG_INFO("WM changed \n");
            }
        }
        else if(BTN_PRESSED == pca10059_GetButtonState() && eNewWM != EWM_NO_INPUT)
        {
            nrf_delay_ms(10);
            if(BTN_PRESSED == pca10059_GetButtonState())
            {
                unTimeCnt+=10;
            }
            else
                unTimeCnt = 0;

            if(unTimeCnt == WAIT_APPLY_PARAM_MS)
            {
                unTimeCnt = 0;
                NRF_LOG_INFO("Setting H %u , S %u , V %u \n", sHSV.H, sHSV.S, sHSV.V);
                IncrementHSVByWormode(&sHSV, eNewWM);

                HSVtoRGB_calc(&sHSV, &sRGB);
                pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);
            }
        }
        else
        {
            unTimeCnt = 0;
        }


        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}



