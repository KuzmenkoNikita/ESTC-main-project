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

#define LED_PWM_PERIOD_US 1000

/**
 * @brief Init logs
 */
void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


void ButtonHandler(eBtnState eState, void* pData)
{
    NRF_LOG_INFO("!!! Double click !!! ");

    if(!pData)
        return;

    bool* pfEnState = (bool*)pData;
    *pfEnState = !(*pfEnState);
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    
    SBlinkParams msBlinkParams[BLINKING_MAX_PARAM_SIZE] = 
                                                    {
                                                        {ELED_1, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 6, 500},
                                                        {ELED_2, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 5, 500},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 7, 500},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_OFF, ECOLOR_ON}, 8, 500}
                                                    }; 

    SBlinkyInstance sInst;
    SBtnIRQParams sBtnIrq;
    bool fEnable = false;


    sBtnIrq.eBtnIrqState    = BTN_DOUBLE_CLICKED;
    sBtnIrq.fnBtnHandler    = ButtonHandler;
    sBtnIrq.pUserData       = (void*)&fEnable;

    logs_init();

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    pca10059_BlinkByParams_init(&sInst, LED_PWM_PERIOD_US, msBlinkParams, BLINKING_MAX_PARAM_SIZE);

    while(1)
    {
        pca10059_BlinkByParams_process(&sInst, fEnable);
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

