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

#define WAIT_AFTER_CHANGE_WM_MS     400
#define WAIT_APPLY_PARAM_MS         200

#define LEDSTATESAVER_FIRST_PAGE_ADDR   0x000DD000
#define LEDSTATESAVER_SECOND_PAGE_ADDR  0x000DE000

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

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}



