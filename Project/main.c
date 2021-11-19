#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_delay.h"
#include "pca10059_led.h"
#include "pca10059_button.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "nrfx_systick.h"
#include <nrfx_timer.h>
#include "pca10059_led_pwm.h"

#include "pca10059_LedsBlinkByParams.h"


#define LED_PWM_PERIOD_US       1000


typedef struct
{
    nrfx_systick_state_t    sDebounceSystick;
    uint32_t             unPressCnt;
    bool                   fLowElapsed;
    
}SDebounceParams;


/**
 * @brief Init logs
 */
void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void log_led_color(ELedNum eLed,  bool fRiseColor)
{
    unsigned LedNum = 0;

    switch(eLed)
    {
        case ELED_1: 
        {
            LedNum = 1; 
            break;
        }
        case ELED_2: 
        {
            LedNum = 2;
            break;
        }

        default: 
        {
            NRF_LOG_INFO("Uknown LED coolor \n"); 
            return;
        }
    }

    if(fRiseColor)
    {
        NRF_LOG_INFO("LED %d stated turning on \n", LedNum);
    }
    else
    {
        NRF_LOG_INFO("LED %d stated turning off \n", LedNum); 
    }
}

void assert_nrf_callback(uint16_t line_num, const uint8_t * file_name)
{
    NRF_LOG_INFO(" ASSERT !!!!!"); 
}


void ButtonHandler(eBtnState eState, void* pData)
{
    bool fHiHiElapsed = false;

    if(pData)
    {
        NRF_LOG_INFO("BTN PRESSED!!!!!!!!!!!!!!!!!!!!!");
        SDebounceParams* psParams = (SDebounceParams*)pData; 

             psParams->unPressCnt++;

            fHiHiElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 10);
            if(fHiHiElapsed)
            {
                nrfx_systick_get(&psParams->sDebounceSystick);
                psParams->unPressCnt = 1;
                NRF_LOG_INFO("time Hi");
            }
            else
            {
                NRF_LOG_INFO("FATAL ERROR FATAL ERROR");
            }

#if 0
        if(psParams->unPressCnt == 1)
        {
            NRF_LOG_INFO("First Click!");
            nrfx_systick_get(&psParams->sDebounceSystick);
        }

        if(psParams->unPressCnt > 1)
        {
            //bool fHiElapsed = false;
            bool fHiHiElapsed = false;
            
            fHiHiElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 120000);
            if(fHiHiElapsed)
            {
                nrfx_systick_get(&psParams->sDebounceSystick);
                psParams->unPressCnt = 1;
                NRF_LOG_INFO("Too late click");
                return;
            }
            else
            {
                NRF_LOG_INFO("Not Elapsed");
            }

            psParams->fLowElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 30000);
            fHiElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 80000);

            NRF_LOG_INFO(" Hi %d, Low %d", fHiElapsed, psParams->fLowElapsed); 

        
            if (psParams->fLowElapsed && fHiElapsed)
            {
                psParams->unPressCnt = 0;
                NRF_LOG_INFO(" Double CLick!!!!");
            }

            //if (!psParams->fLowElapsed)
            //{
                //--psParams->unPressCnt;
            //}
        }
        else
        {
            NRF_LOG_INFO("ELSE Cnt %u", psParams->unPressCnt);
        }
#endif
    } 
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    
    SBlinkParams msBlinkParams[PCA10059_DEVID_SIZE] = 
                                                    {
                                                        {ELED_1, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 6, 500, LED_PWM_PERIOD_US},
                                                        {ELED_2, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 5, 500, LED_PWM_PERIOD_US},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 7, 500, LED_PWM_PERIOD_US},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_OFF, ECOLOR_ON}, 8, 500, LED_PWM_PERIOD_US}
                                                    }; 

    SDebounceParams sBtnIqrParams;
    sBtnIqrParams.unPressCnt = 0;
    SBlinkyInstance sInst;
    SBtnIRQParams sBtnIrq;
    
    sBtnIrq.eBtnIrqState = BTN_PRESSED;
    sBtnIrq.fnBtnHandler = ButtonHandler;
    sBtnIrq.pUserData = (void*)&sBtnIqrParams;

    nrfx_systick_init();

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    pca10059_BlinkByParams_init(&sInst, msBlinkParams, PCA10059_DEVID_SIZE);

    while(1)
    {
        pca10059_BlinkByParams_process(&sInst, true);
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

