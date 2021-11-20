#ifndef PCA10059_BLINKY_BY_PRAMS
#define PCA10059_BLINKY_BY_PRAMS

#include <stdint.h>
#include "pca10059_led.h"
#include "pca10059_led_pwm.h"

#define BLINKING_MAX_PARAM_SIZE    4

/** @brief Blinking init params */
typedef struct
{
    ELedNum     eLed;               /* Led for blinking */
    SLedColors  sColor;             /* Led color */
    uint32_t    BlinksCnt;          /* One led blink counter */
    uint32_t    BlinkTimeMs;        /* Blink time in miliseconds */
}SBlinkParams;


/** @brief module instance struct*/
typedef struct  
{
    SBlinkParams        msBlinkParams[BLINKING_MAX_PARAM_SIZE];
    SLedPwmTimeParams   sLedTimeParams;
    Spca10059_led_pwm   sLed1PWM;
    Spca10059_led_pwm   sLed2PWM;
    uint32_t            unTotalTimeUs;
    uint32_t            unLedsPWMPeriodUs;
    uint32_t            unBlinkCnt;
    uint32_t            i;
    bool                fRiseColor;
    uint32_t            unMassParamSize;
}SBlinkyInstance;

/**
 * @brief Module init
 *
 * @param psInstance            pointer to module instance struct
 * @param unLedsPwmPeriodUs     PWM period for blinking in microseconds
 * @param psBlinkyParams        pointer to mass with blinking params. Blink order depends on order params in array.
 * @param unMassSize            Count of blinking params. Shouldn't be more than PCA10059_DEVID_SIZE
 */
void pca10059_BlinkByParams_init(SBlinkyInstance* psInstance, uint32_t unLedsPwmPeriodUs, SBlinkParams* psBlinkyParams, uint32_t unMassSize);

/**
 * @brief Blinking processing
 *
 * @param psInstance            pointer to module instance struct
 * @param fEnState              blinking enable state
 */
void pca10059_BlinkByParams_process(SBlinkyInstance* psInstance, bool fEnState);

#endif /* PCA10059_BLINKY_BY_PRAMS */