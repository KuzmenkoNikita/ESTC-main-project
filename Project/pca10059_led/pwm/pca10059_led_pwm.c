#include "pca10059_led_pwm.h"
#include <stdlib.h>

typedef enum
{
    PWM_COLOR_RED,
    PWM_COLOR_GREEN,
    PWM_COLOR_BLUE
}EPwmColors;

void pca10059_led_pwm_init(Spca10059_led_pwm* psLedPwm, uint32_t PeriodUsec, ELedNum eLed)
{
    unsigned i = 0;

    if(!psLedPwm)
        return;

    nrfx_systick_init();
    pca10059_leds_init();

    for(i = 0; i < PWM_COUNTOF_LED_COLORS; ++i)
    {
        nrfx_systick_get(&psLedPwm->sColorPwmParams[i].sSysTickState);
        psLedPwm->sColorPwmParams[i].eLedState = ECOLOR_OFF;
        psLedPwm->sColorPwmParams[i].unTicksCnt = 0;
        psLedPwm->sColorPwmParams[i].unTimeOnUsec = 0;
    }

    psLedPwm->PeriodUsec        = PeriodUsec;
    psLedPwm->eLed              = eLed;
}
/* **************************************************** */
void pca10059_led_pwm_set_params(Spca10059_led_pwm* psLedPwm, const SLedPwmTimeParams* psParams)
{
    if(!psLedPwm || !psParams)
        return;

    psLedPwm->sColorPwmParams[PWM_COLOR_BLUE].unTimeOnUsec  = MIN (psParams->unBlueTOnUsec, psLedPwm->PeriodUsec);
    psLedPwm->sColorPwmParams[PWM_COLOR_GREEN].unTimeOnUsec = MIN (psParams->unGreenTOnUsec, psLedPwm->PeriodUsec);
    psLedPwm->sColorPwmParams[PWM_COLOR_RED].unTimeOnUsec   = MIN (psParams->unRedTOnUsec, psLedPwm->PeriodUsec);
}
/* **************************************************** */
void pca10059_led_pwm_process(Spca10059_led_pwm* psLedPwm)
{
    SLedColors sColors;
    unsigned i = 0;

    if(!psLedPwm)
        return;

    for(i = 0; i < PWM_COUNTOF_LED_COLORS; ++i)
    {
        if(psLedPwm->sColorPwmParams[i].unTimeOnUsec != 0)
        {
            if (nrfx_systick_test(&psLedPwm->sColorPwmParams[i].sSysTickState, psLedPwm->sColorPwmParams[i].unTicksCnt))
            {
                if(psLedPwm->sColorPwmParams[i].eLedState == ECOLOR_ON)
                {
                    psLedPwm->sColorPwmParams[i].eLedState = ECOLOR_OFF;
                    psLedPwm->sColorPwmParams[i].unTicksCnt = psLedPwm->PeriodUsec - psLedPwm->sColorPwmParams[i].unTimeOnUsec;
                }
                else
                {
                    psLedPwm->sColorPwmParams[i].eLedState = ECOLOR_ON;
                    psLedPwm->sColorPwmParams[i].unTicksCnt = psLedPwm->sColorPwmParams[i].unTimeOnUsec; 
                }

                nrfx_systick_get(&psLedPwm->sColorPwmParams[i].sSysTickState);
            }
        }
        else
            psLedPwm->sColorPwmParams[i].eLedState = ECOLOR_OFF;
    }
    
    sColors.eBlueState  = psLedPwm->sColorPwmParams[PWM_COLOR_BLUE].eLedState;
    sColors.eRedState   = psLedPwm->sColorPwmParams[PWM_COLOR_RED].eLedState;
    sColors.eGreenState = psLedPwm->sColorPwmParams[PWM_COLOR_GREEN].eLedState;

    pca10059_LedSetColor(psLedPwm->eLed, &sColors);
}


