#ifndef PCA10059_BLINKY_BY_PRAMS
#define PCA10059_BLINKY_BY_PRAMS

#include <stdint.h>
#include "pca10059_led.h"
#include "pca10059_led_pwm.h"

#define PCA10059_DEVID_SIZE     4

/** @brief Blinking init params */
typedef struct
{
    ELedNum     eLed;
    SLedColors  sColor;
    uint32_t    BlinksCnt;
    uint32_t    BlinkTimeMs;
    uint32_t    unLedsPWMPeriodUs;
}SBlinkParams;


typedef struct  
{
    SBlinkParams        msBlinkParams[PCA10059_DEVID_SIZE];
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


void pca10059_BlinkByParams_init(SBlinkyInstance* psInstance, SBlinkParams* psBlinkyParams, uint32_t unMassSize);

void pca10059_BlinkByParams_process(SBlinkyInstance* psInstance, bool fEnState);

#endif /* PCA10059_BLINKY_BY_PRAMS */