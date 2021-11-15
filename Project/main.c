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

#include "pca10059_led_pwm.h"

#define PCA10059_DEVID_SIZE     4
#define LED_PWM_PERIOD_US       1000

/** @brief Blinking params */
typedef struct
{
    ELedNum     eLed;
    SLedColors  sColor;
    uint32_t    BlinksCnt;
    uint32_t    BlinkTimeMs;
}SBlinkParams;

typedef struct 
{
    ELedStete   eledState;
    uint32_t*   pTimeOn;
}SOneColorPwm;

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

void ButtonHandler(eBtnState eState, void* pData)
{
    if(pData)
    {
        
        uint32_t* pCnt = (uint32_t*)pData;
        ++(*pCnt);
        NRF_LOG_INFO("Pressed CNT %d", *pCnt); 
    } 
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t unPressBtnCnt = 0;
    unsigned int unTotalTimeUs = 0;
    unsigned int i = 0;
    unsigned int unBlinkCnt = 0;
    SBlinkParams msBlinkParams[PCA10059_DEVID_SIZE] = 
                                                    {
                                                        {ELED_1, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 6, 1000},
                                                        {ELED_2, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 5, 1000},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 7, 1000},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_OFF, ECOLOR_ON}, 8, 1000}
                                                    };
    bool fRiseColor = true;
    SLedPwmTimeParams sLedTimeParams = {0,0,0};
    uint32_t unPWMStep = 0;
    bool fEnState = false;
    Spca10059_led_pwm sLed1PWM;
    Spca10059_led_pwm sLed2PWM;

    SBtnIRQParams sBtnIrq;

    sBtnIrq.eBtnIrqState = BTN_PRESSED;
    sBtnIrq.fnBtnHandler = ButtonHandler;
    sBtnIrq.pUserData = (void*)&unPressBtnCnt;

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();
    logs_init();



    pca10059_led_pwm_init(&sLed1PWM, LED_PWM_PERIOD_US, ELED_1);
    pca10059_led_pwm_init(&sLed2PWM, LED_PWM_PERIOD_US, ELED_2);

    sLedTimeParams.unGreenTOnUsec   = 0;
    sLedTimeParams.unBlueTOnUsec    = 0;
    sLedTimeParams.unRedTOnUsec     = 0;
    
    pca10059_led_pwm_set_params(&sLed1PWM, &sLedTimeParams);
    pca10059_led_pwm_set_params(&sLed2PWM, &sLedTimeParams);

    while(1)
    {
        if(unPressBtnCnt == 2)
        {
            unPressBtnCnt = 0;
            fEnState = !fEnState;
        }

        pca10059_led_pwm_process(&sLed1PWM);
        pca10059_led_pwm_process(&sLed2PWM);

        if(/*BTN_PRESSED == pca10059_GetButtonState()*/fEnState)
        {
            if(unTotalTimeUs == 1000 * msBlinkParams[i].BlinkTimeMs)
            {
                fRiseColor = false;
                log_led_color(msBlinkParams[i].eLed, fRiseColor);
            }
            else if (unTotalTimeUs == 2*1000 * msBlinkParams[i].BlinkTimeMs)
            {
                unTotalTimeUs = 0;
                ++unBlinkCnt;
                if(unBlinkCnt == msBlinkParams[i].BlinksCnt)
                {
                    unBlinkCnt = 0;
                    sLedTimeParams.unGreenTOnUsec = 0;
                    sLedTimeParams.unBlueTOnUsec = 0;
                    sLedTimeParams.unRedTOnUsec = 0;

                    ++i;
                    if(i == PCA10059_DEVID_SIZE)
                        i = 0;
                }

                fRiseColor = true;
                log_led_color(msBlinkParams[i].eLed, fRiseColor);
            }

            unPWMStep = (1000 * msBlinkParams[i].BlinkTimeMs) / LED_PWM_PERIOD_US;

            if(unTotalTimeUs % unPWMStep == 0)
            {
                unsigned j = 0;
                SOneColorPwm msParams[PWM_COUNTOF_LED_COLORS] = {
                                                                    {msBlinkParams[i].sColor.eGreenState, &sLedTimeParams.unGreenTOnUsec},
                                                                    {msBlinkParams[i].sColor.eBlueState, &sLedTimeParams.unBlueTOnUsec},
                                                                    {msBlinkParams[i].sColor.eRedState, &sLedTimeParams.unRedTOnUsec},
                                                                };
                for(j = 0; j < PWM_COUNTOF_LED_COLORS; ++j)
                    if(msParams[j].eledState == ECOLOR_ON)
                        *msParams[j].pTimeOn = fRiseColor ? (*msParams[j].pTimeOn) + 1 : (*msParams[j].pTimeOn ) - 1;
            }
            
            if(msBlinkParams[i].eLed == ELED_1)
                pca10059_led_pwm_set_params(&sLed1PWM, &sLedTimeParams);
            else
                pca10059_led_pwm_set_params(&sLed2PWM, &sLedTimeParams);

            nrf_delay_us(100);

            unTotalTimeUs+=100;
        }
        
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

