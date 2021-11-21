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

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    while(1)
    {
        NRF_LOG_INFO("ERROR HANDLER !!!! ");
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

void ButtonHandler(eBtnState eState, void* pData)
{
    EWMTypes mWMs[3] = {EWM_TUNING_H, EWM_TUNING_S, EWM_TUNING_V};

    if(!pData)
        return;

    uint32_t* pCnt = (uint32_t*)pData;
    WMIndication_SetWM(mWMs[*pCnt]);

    (*pCnt)++;

    if(*pCnt == 3)
        *pCnt = 0;
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{

    SBtnIRQParams sBtnIrq;
    //bool fEnable = false;
    uint32_t unCnt = 0;

    sBtnIrq.eBtnIrqState    = BTN_DOUBLE_CLICKED;
    sBtnIrq.fnBtnHandler    = ButtonHandler;
    sBtnIrq.pUserData       = (void*)&unCnt;
    

    logs_init();

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    pca10059_RGBLed_init();
    pca10059_RGBLed_Set(255, 255, 255);

    WMIndication_init();

    WMIndication_SetWM(EWM_NO_INPUT);

    while(1)
    {
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

