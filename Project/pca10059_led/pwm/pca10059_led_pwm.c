#include "pca10059_led_pwm.h"


void pca10059_led_pwm_init(Spca10059_led_pwm* psLedPwm, uint32_t PeriodUsec, ELedNum eLed)
{
    if(!psLedPwm)
        return;

    nrfx_systick_init();
    pca10059_leds_init();

    nrfx_systick_get(&psLedPwm->sGreenTickState);
    nrfx_systick_get(&psLedPwm->sBlueTickState);
    nrfx_systick_get(&psLedPwm->sRedTickState);

    psLedPwm->PeriodUsec        = PeriodUsec;
    psLedPwm->eLed              = eLed;

    psLedPwm->unGreenTickCnt    = 0;
    psLedPwm->unBlueTickCnt     = 0;
    psLedPwm->unRedTickCnt      = 0;
    psLedPwm->sTimeParams.unGreenTOnUsec = 0;
    psLedPwm->sTimeParams.unBlueTOnUsec = 0;
    psLedPwm->sTimeParams.unRedTOnUsec = 0;

    psLedPwm->sColors.eGreenState   = ECOLOR_OFF;
    psLedPwm->sColors.eRedState     = ECOLOR_OFF;
    psLedPwm->sColors.eBlueState    = ECOLOR_OFF;
}
/* **************************************************** */
void pca10059_led_pwm_set_params(Spca10059_led_pwm* psLedPwm, const SLedPwmTimeParams* psParams)
{
    if(!psLedPwm || !psParams)
        return;

    if(psParams->unBlueTOnUsec > psLedPwm->PeriodUsec)
        psLedPwm->sTimeParams.unBlueTOnUsec = psLedPwm->PeriodUsec;
    else
        psLedPwm->sTimeParams.unBlueTOnUsec = psParams->unBlueTOnUsec;

    if(psParams->unGreenTOnUsec > psLedPwm->PeriodUsec)
        psLedPwm->sTimeParams.unGreenTOnUsec = psLedPwm->PeriodUsec;
    else
        psLedPwm->sTimeParams.unGreenTOnUsec = psParams->unGreenTOnUsec;

    if(psParams->unRedTOnUsec > psLedPwm->PeriodUsec)
        psLedPwm->sTimeParams.unRedTOnUsec = psLedPwm->PeriodUsec;
    else
        psLedPwm->sTimeParams.unRedTOnUsec = psParams->unRedTOnUsec;
    
}
/* **************************************************** */
void pca10059_led_pwm_process(Spca10059_led_pwm* psLedPwm)
{
    if(!psLedPwm)
        return;

    if(psLedPwm->sTimeParams.unGreenTOnUsec != 0)
    {
        if (nrfx_systick_test(&psLedPwm->sGreenTickState, psLedPwm->unGreenTickCnt))
        {
            if(psLedPwm->sColors.eGreenState == ECOLOR_ON)
            {
                psLedPwm->sColors.eGreenState = ECOLOR_OFF;
                psLedPwm->unGreenTickCnt = psLedPwm->PeriodUsec - psLedPwm->sTimeParams.unGreenTOnUsec;
            }
            else
            {
                psLedPwm->sColors.eGreenState = ECOLOR_ON;
                psLedPwm->unGreenTickCnt = psLedPwm->sTimeParams.unGreenTOnUsec; 
            }

            nrfx_systick_get(&psLedPwm->sGreenTickState);

            //pca10059_LedSetColor(psLedPwm->eLed, &psLedPwm->sColors);
        }
    }
    else
        psLedPwm->sColors.eGreenState = ECOLOR_OFF;


    if (nrfx_systick_test(&psLedPwm->sBlueTickState, psLedPwm->unBlueTickCnt))
    {
        if(psLedPwm->sColors.eBlueState == ECOLOR_ON)
        {
            psLedPwm->sColors.eBlueState = ECOLOR_OFF;
            psLedPwm->unBlueTickCnt = psLedPwm->PeriodUsec - psLedPwm->sTimeParams.unBlueTOnUsec;
        }
        else
        {
            psLedPwm->sColors.eBlueState = ECOLOR_ON;
            psLedPwm->unBlueTickCnt = psLedPwm->sTimeParams.unBlueTOnUsec; 
        }

        nrfx_systick_get(&psLedPwm->sBlueTickState);

        //pca10059_LedSetColor(psLedPwm->eLed, &psLedPwm->sColors);
    }

    if (nrfx_systick_test(&psLedPwm->sRedTickState, psLedPwm->unRedTickCnt))
    {
        if(psLedPwm->sColors.eRedState == ECOLOR_ON)
        {
            psLedPwm->sColors.eRedState = ECOLOR_OFF;
            psLedPwm->unRedTickCnt = psLedPwm->PeriodUsec - psLedPwm->sTimeParams.unRedTOnUsec;
        }
        else
        {
            psLedPwm->sColors.eRedState = ECOLOR_ON;
            psLedPwm->unRedTickCnt = psLedPwm->sTimeParams.unRedTOnUsec; 
        }

        nrfx_systick_get(&psLedPwm->sRedTickState);

        //pca10059_LedSetColor(psLedPwm->eLed, &psLedPwm->sColors);
    }

    pca10059_LedSetColor(psLedPwm->eLed, &psLedPwm->sColors);
}


