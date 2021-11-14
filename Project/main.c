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
#define LED_PWM_PERIOD_US       20000

/** @brief Blinking params */
typedef struct
{
    ELedNum     eLed;
    SLedColors  sColor;
    uint32_t    BlinksCnt;
    uint32_t    BlinkTimems;
}SBlinkParams;


/**
 * @brief Function for blinking led
 * @param eLed Led num for blink
 * @param eColor Led color
 * @param unCnt count of blinks
 */
void led_blink(SBlinkParams* psBlinkParam)
{
    #if 0
    if(!psBlinkParam)
        return;

    for(int i = 0; i < psBlinkParam->BlinksCnt; ++i)
    {
        pca10059_LedSetColor(psBlinkParam->eLed, psBlinkParam->eColor);

        nrf_delay_ms(psBlinkParam->BlinkTimems);

        pca10059_LedSetColor(psBlinkParam->eLed, ECOLOR_OFF);

        nrf_delay_ms(psBlinkParam->BlinkTimems);
    }
    #endif
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

void log_led_color(ELedNum eLed/*,  ELedColor eColor*/)
{
    #if 0
    unsigned LedNum = 0;
    char* Color_str = 0;
    
    switch(eColor)
    {
        case ECOLOR_OFF:    Color_str = "Led color OFF, LED num: %u \n"; break;
        case ECOLOR_GREEN:  Color_str = "Led color GREEN, LED num: %u \n"; break;  
        case ECOLOR_RED:    Color_str = "Led color RED, LED num: %u \n"; break;
        case ECOLOR_BLUE:   Color_str = "Led color BLUE, LED num: %u \n"; break;
        case ECOLOR_ORANGE: Color_str = "Led color ORANGE, LED num: %u \n"; break;
        default: NRF_LOG_INFO("Uknown LED coolor \n"); return;
    }

    switch(eLed)
    {
        case ELED_1: LedNum = 1; break;
        case ELED_2: LedNum = 2; break;
        default: NRF_LOG_INFO("Uknown LED coolor \n"); return;
    }

    NRF_LOG_INFO(Color_str, LedNum);
    #endif
}


/**
 * @brief Function for application main entry.
 */
int main(void)
{
    
    unsigned int unTotalTime = 0;
    unsigned int i = 0;
    unsigned int unBlinkCnt = 0;
    SBlinkParams msBlinkParams[PCA10059_DEVID_SIZE] = 
                                                    {
                                                        {ELED_1, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 6, 1000},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 5, 1000},
                                                        {ELED_2, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 7, 1000},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_OFF, ECOLOR_ON}, 8, 1000}
                                                    };
    //ELedColor eColor = msBlinkParams[0].eColor;
    bool fRiseColor = true;
    
    //nrfx_systick_state_t systickState;
    //uint32_t  testVal = 1000;

    //ELedColor eColor2 = ECOLOR_BLUE;

    //uint32_t  testVal2 = 100;
    //nrfx_systick_state_t systickState2;
    //SLedColors sColors;
    SLedPwmTimeParams sLedTimeParams = {0,0,0};
    //SLedPwmTimeParams sLedTimeParams2;

    //pca10059_leds_init();
    pca10059_button_init();
    logs_init();

    //sColors.eGreenState     = ECOLOR_OFF;
    //sColors.eRedState       = ECOLOR_ON;
    //sColors.eBlueState      = ECOLOR_OFF;

    //pca10059_LedSetColor(ELED_2, &sColors);

    //nrfx_systick_init();

    //nrfx_systick_get(&systickState);
    //nrfx_systick_get(&systickState2);

    Spca10059_led_pwm sLed1PWM;
    Spca10059_led_pwm sLed2PWM;

    pca10059_led_pwm_init(&sLed1PWM, LED_PWM_PERIOD_US, ELED_1);
    pca10059_led_pwm_init(&sLed2PWM, LED_PWM_PERIOD_US, ELED_2);

    sLedTimeParams.unGreenTOnUsec   = 0;
    sLedTimeParams.unBlueTOnUsec    = 0;
    sLedTimeParams.unRedTOnUsec     = 0;
    
    pca10059_led_pwm_set_params(&sLed1PWM, &sLedTimeParams);
    pca10059_led_pwm_set_params(&sLed2PWM, &sLedTimeParams);

    while(1)
    {
        pca10059_led_pwm_process(&sLed1PWM);
        pca10059_led_pwm_process(&sLed2PWM);

        if(BTN_PRESSED == pca10059_GetButtonState())
        {
            if(unTotalTime == msBlinkParams[i].BlinkTimems)
            {
                fRiseColor = false;
                //eColor = ECOLOR_OFF;
            }
            else if (unTotalTime == 2 * msBlinkParams[i].BlinkTimems)
            {
                unTotalTime = 0;
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
                //eColor = msBlinkParams[i].eColor;
            }

            if(msBlinkParams[i].sColor.eGreenState == ECOLOR_ON)
            {
                if(fRiseColor)
                    sLedTimeParams.unGreenTOnUsec += LED_PWM_PERIOD_US / msBlinkParams[i].BlinkTimems;
                else
                    sLedTimeParams.unGreenTOnUsec -= LED_PWM_PERIOD_US / msBlinkParams[i].BlinkTimems;
            }

            if(msBlinkParams[i].sColor.eBlueState == ECOLOR_ON)
            {
                if(fRiseColor)
                    sLedTimeParams.unBlueTOnUsec += LED_PWM_PERIOD_US / msBlinkParams[i].BlinkTimems;
                else
                    sLedTimeParams.unBlueTOnUsec -= LED_PWM_PERIOD_US / msBlinkParams[i].BlinkTimems;
            }

            if(msBlinkParams[i].sColor.eRedState == ECOLOR_ON)
            {
                if(fRiseColor)
                    sLedTimeParams.unRedTOnUsec += LED_PWM_PERIOD_US / msBlinkParams[i].BlinkTimems;
                else
                    sLedTimeParams.unBlueTOnUsec -= LED_PWM_PERIOD_US / msBlinkParams[i].BlinkTimems;
            }
                //ChangeColor = !ChangeColor; 
                //log_led_color(msBlinkParams[i].eLed, eColor);
                //pca10059_LedSetColor(msBlinkParams[i].eLed, eColor);
            
            if(msBlinkParams[i].eLed == ELED_1)
                pca10059_led_pwm_set_params(&sLed1PWM, &sLedTimeParams);
            else
                pca10059_led_pwm_set_params(&sLed2PWM, &sLedTimeParams);

            nrf_delay_ms(1);

            ++unTotalTime;
        }
        
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

