#include "pca10059_LedsBlinkByParams.h"
#include "pca10059_led_pwm.h"
#include "nrf_delay.h"
#include <string.h>

#define CONVERT_MS2US(ms) (1000 * (ms))

typedef struct 
{
    ELedStete   eledState;
    uint32_t*   pTimeOn;
}SOneColorPwm;

void pca10059_BlinkByParams_init(SBlinkyInstance* psInstance, SBlinkParams* psBlinkyParams, uint32_t unMassSize)
{
    if(!psInstance || !psBlinkyParams)
        return;

    if(unMassSize > PCA10059_DEVID_SIZE)
        return;

    memcpy(&psInstance->msBlinkParams, psBlinkyParams, sizeof(SBlinkParams) * unMassSize);

    psInstance->unLedsPWMPeriodUs = psBlinkyParams->unLedsPWMPeriodUs;
    psInstance->unBlinkCnt = 0;
    psInstance->fRiseColor = true;
    psInstance->i = 0;
    psInstance->unMassParamSize = unMassSize;
    psInstance->unTotalTimeUs = 0;
        
    pca10059_led_pwm_init(&psInstance->sLed1PWM, psBlinkyParams->unLedsPWMPeriodUs, ELED_1);
    pca10059_led_pwm_init(&psInstance->sLed2PWM, psBlinkyParams->unLedsPWMPeriodUs, ELED_2);

    psInstance->sLedTimeParams.unGreenTOnUsec   = 0;
    psInstance->sLedTimeParams.unBlueTOnUsec    = 0;
    psInstance->sLedTimeParams.unRedTOnUsec     = 0;
    
    pca10059_led_pwm_set_params(&psInstance->sLed1PWM, &psInstance->sLedTimeParams);
    pca10059_led_pwm_set_params(&psInstance->sLed2PWM, &psInstance->sLedTimeParams);
}

/* ********************************************************************* */

void pca10059_BlinkByParams_process(SBlinkyInstance* psInstance, bool fEnState)
{
    uint32_t unPWMStep = 0;

    if(!psInstance)
        return;

    pca10059_led_pwm_process(&psInstance->sLed1PWM);
    pca10059_led_pwm_process(&psInstance->sLed2PWM);

    if(fEnState)
    {
        if(psInstance->unTotalTimeUs == CONVERT_MS2US(psInstance->msBlinkParams[psInstance->i].BlinkTimeMs))
        {
            psInstance->fRiseColor = false;
        }
        else if (psInstance->unTotalTimeUs == 2*(CONVERT_MS2US (psInstance->msBlinkParams[psInstance->i].BlinkTimeMs)))
        {
            psInstance->unTotalTimeUs = 0;
            ++psInstance->unBlinkCnt;
            if(psInstance->unBlinkCnt == psInstance->msBlinkParams[psInstance->i].BlinksCnt)
            {
                psInstance->unBlinkCnt = 0;
                psInstance->sLedTimeParams.unGreenTOnUsec = 0;
                psInstance->sLedTimeParams.unBlueTOnUsec = 0;
                psInstance->sLedTimeParams.unRedTOnUsec = 0;

                ++psInstance->i;
                if(psInstance->i == psInstance->unMassParamSize)
                {
                    psInstance->i = 0;
                }
            }

            psInstance->fRiseColor = true;
        }

        unPWMStep = CONVERT_MS2US(psInstance->msBlinkParams[ psInstance->i].BlinkTimeMs) / psInstance->unLedsPWMPeriodUs;

        if(psInstance->unTotalTimeUs % unPWMStep == 0)
        {
            unsigned j = 0;
            SOneColorPwm msParams[PWM_COUNTOF_LED_COLORS] = {
                                                                {psInstance->msBlinkParams[psInstance->i].sColor.eGreenState, &psInstance->sLedTimeParams.unGreenTOnUsec},
                                                                {psInstance->msBlinkParams[psInstance->i].sColor.eBlueState, &psInstance->sLedTimeParams.unBlueTOnUsec},
                                                                {psInstance->msBlinkParams[psInstance->i].sColor.eRedState, &psInstance->sLedTimeParams.unRedTOnUsec},
                                                            };
            for(j = 0; j < PWM_COUNTOF_LED_COLORS; ++j)
                if(msParams[j].eledState == ECOLOR_ON)
                    *msParams[j].pTimeOn = psInstance->fRiseColor ? (*msParams[j].pTimeOn) + 1 : (*msParams[j].pTimeOn ) - 1;
        }
        
        if(psInstance->msBlinkParams[ psInstance->i].eLed == ELED_1)
            pca10059_led_pwm_set_params(&psInstance->sLed1PWM, &psInstance->sLedTimeParams);
        else
            pca10059_led_pwm_set_params(&psInstance->sLed2PWM, &psInstance->sLedTimeParams);

        nrf_delay_us(5);

        psInstance->unTotalTimeUs+=5;
    }
}