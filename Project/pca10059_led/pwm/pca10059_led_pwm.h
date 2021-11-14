#ifndef PCA10059_LED_PWM
#define PCA10059_LED_PWM

#include <stdint.h>
#include "nrfx_systick.h"
#include "pca10059_led.h"

#define PWM_COUNTOF_LED_COLORS  3

/** @brief structure for setting pwm led params */
typedef struct 
{
    uint32_t             unGreenTOnUsec;    /* Green color mark time */
    uint32_t             unBlueTOnUsec;     /* Green color mark time */
    uint32_t             unRedTOnUsec;      /* Red color mark time */
}SLedPwmTimeParams;

/** @brief structure contains params for one pwm cycle */
typedef struct
{
    nrfx_systick_state_t    sSysTickState;  
    uint32_t                unTimeOnUsec;
    uint32_t                unTicksCnt;
    ELedStete               eLedState;
}SColorPWMParams;

/** @brief main LED PWM params */
typedef struct 
{
    SColorPWMParams         sColorPwmParams[PWM_COUNTOF_LED_COLORS];
    uint32_t                PeriodUsec;    
    ELedNum                 eLed;
}Spca10059_led_pwm;

/**
 * @brief LED PWM module init
 *
 * @param psLedPwm      pointer to main pwm structure
 * @param PeriodUsec    pwm period in usecs
 * @param eLed          assigned led
 */
void pca10059_led_pwm_init(Spca10059_led_pwm* psLedPwm, uint32_t PeriodUsec, ELedNum eLed);

/**
 * @brief Setting PWM time params
 *
 * @param psLedPwm      pointer to main pwm structure
 * @param psParams      poiner to structure with PWM time params
 */
void pca10059_led_pwm_set_params(Spca10059_led_pwm* psLedPwm, const SLedPwmTimeParams* psParams);

/**
 * @brief Process PWM cycles. Should be called periodically
 *
 * @param psLedPwm      pointer to main pwm structure
 */
void pca10059_led_pwm_process(Spca10059_led_pwm* psLedPwm);

#endif