#ifndef PCA10059_LED_PWM
#define PCA10059_LED_PWM

#include <stdint.h>
#include "nrfx_systick.h"
#include "pca10059_led.h"


typedef struct 
{
    uint32_t             unGreenTOnUsec;
    uint32_t             unBlueTOnUsec;
    uint32_t             unRedTOnUsec;
}SLedPwmTimeParams;

typedef struct 
{
    nrfx_systick_state_t    sGreenTickState;
    nrfx_systick_state_t    sBlueTickState;
    nrfx_systick_state_t    sRedTickState;
    SLedColors              sColors;
    SLedPwmTimeParams       sTimeParams;
    uint32_t                PeriodUsec;    
    ELedNum                 eLed;
    uint32_t                unGreenTickCnt;
    uint32_t                unBlueTickCnt;
    uint32_t                unRedTickCnt;
}Spca10059_led_pwm;

void pca10059_led_pwm_init(Spca10059_led_pwm* psLedPwm, uint32_t PeriodUsec, ELedNum eLed);

void pca10059_led_pwm_set_params(Spca10059_led_pwm* psLedPwm, const SLedPwmTimeParams* psParams);

void pca10059_led_pwm_process(Spca10059_led_pwm* psLedPwm);

#endif