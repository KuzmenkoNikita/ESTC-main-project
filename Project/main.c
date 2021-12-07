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
#include "app_usbd_cdc_acm.h"
#include "LEDState_Parser.h"

#define WAIT_AFTER_CHANGE_WM_MS     400
#define WAIT_APPLY_PARAM_MS         200

#define LEDSTATESAVER_FIRST_PAGE_ADDR   0x000DD000
#define LEDSTATESAVER_SECOND_PAGE_ADDR  0x000DE000

#define READ_SIZE 1

static char m_rx_buffer[READ_SIZE];
const char szUnknownCMD[]="Unknown command! Try \"help\" for info\n\r"; 
const char szHelpHSVMsg[]="HSV <H> <S> <V> - Set HSV LED color H = [0:360], S = [0:100], V = [0:100]\n\r";
const char szHelpRGBMsg[]="RGB <R> <G> <B> - Set RGB LED color R = [0:255], G = [0:255], B = [0:255]\n\r";
const char szHelpMsg[]= "help - print help message\n\r";

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4

APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                            usb_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

typedef struct 
{
    SHSVCoordinates*    psHSV;
    EWMTypes*           peCurrentWM;
    SLedStateSaverInst* psSaver;
}SButtonIRQData;

static SLEDStateParserInst sParserInst;
bool fCanSend = true;

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event)
{
    switch (event)
    {
    case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
    {
        ret_code_t ret;
        ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
        UNUSED_VARIABLE(ret);
        break;
    }

    case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
    {
        NRF_LOG_INFO("TX DONE \n");
        fCanSend = true;
        break;
    }

    case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
    {
        ret_code_t ret;
        do
        {
            if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 2);
            }
            else
            {
                ret = app_usbd_cdc_acm_write(&usb_cdc_acm,
                                             m_rx_buffer,
                                             READ_SIZE);
            }

            LEDStateParser_PutByte(&sParserInst, m_rx_buffer[0]);

            ret = app_usbd_cdc_acm_read(&usb_cdc_acm,
                                        m_rx_buffer,
                                        READ_SIZE);
        } while (ret == NRF_SUCCESS);

        break;
    }
    default:
        break;
    }
}


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
/* ********************************************************************************************* */
void LEDStateParser_HelpRequest(void* pData)
{
    while(NRFX_SUCCESS != app_usbd_cdc_acm_write(&usb_cdc_acm, szHelpMsg, strlen(szHelpMsg)));
    while(NRFX_SUCCESS != app_usbd_cdc_acm_write(&usb_cdc_acm, szHelpHSVMsg, strlen(szHelpHSVMsg)));
    while(NRFX_SUCCESS != app_usbd_cdc_acm_write(&usb_cdc_acm, szHelpRGBMsg, strlen(szHelpRGBMsg)));
}
/* ********************************************************************************************* */
void LEDStateParser_CMDError(void* pData)
{
    while(NRFX_SUCCESS != app_usbd_cdc_acm_write(&usb_cdc_acm, szUnknownCMD,strlen(szUnknownCMD)));
}
/* ********************************************************************************************* */
void LEDStateParser_SetLedState(ULEDStateParams* puParams, ETypeParams eTypeParam, void* pData)
{
    SRGBCoordinates sRGB;
    char szColorstr[50];
    SButtonIRQData* psIrqData = (SButtonIRQData*)pData;
    if(!psIrqData)
        return;

    switch(eTypeParam)
    {
        case ELEDPARSER_HSV:
        {
            sprintf(szColorstr, "Color set to H:%u S:%u V:%u\n\r", puParams->sHSV.H,
                                                                puParams->sHSV.S,
                                                                puParams->sHSV.V);

            psIrqData->psHSV->H = puParams->sHSV.H;
            psIrqData->psHSV->S = puParams->sHSV.S;
            psIrqData->psHSV->V = puParams->sHSV.V;

            HSVtoRGB_calc(psIrqData->psHSV, &sRGB);
            pca10059_RGBLed_Set(sRGB.R, sRGB.G, sRGB.B);

            break;
        }

        case ELEDPARSER_RGB:
        {
            NRF_LOG_INFO("RGB: R: %u G: %u B: %u \n", puParams->sRGB.R, puParams->sRGB.G, puParams->sRGB.B);
            pca10059_RGBLed_Set(puParams->sRGB.R, puParams->sRGB.G, puParams->sRGB.B);
            break;
        }
    }

    while(NRFX_SUCCESS != app_usbd_cdc_acm_write(&usb_cdc_acm, szColorstr,strlen(szColorstr)));

    SLEDColorState sLedColorState;

    sLedColorState.H    = psIrqData->psHSV->H;
    sLedColorState.S    = psIrqData->psHSV->S;
    sLedColorState.V    = psIrqData->psHSV->V;

    LedStateSaver_SaveLedState(psIrqData->psSaver, &sLedColorState);
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

    
    SLEDStateParserInfo sParserInfo;

    sParserInfo.fnHelpRequest   = LEDStateParser_HelpRequest;
    sParserInfo.fnCMDError      = LEDStateParser_CMDError;
    sParserInfo.fnSetState      = LEDStateParser_SetLedState;
    sParserInfo.pData           = (void*)&sBtnIRQData;

    LEDStateParser_init(&sParserInst, &sParserInfo);
    
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
    ret_code_t ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    while(1)
    {
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

        while(app_usbd_event_queue_process())
        {
            
        }

        LEDStateParser_Process(&sParserInst);
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}



