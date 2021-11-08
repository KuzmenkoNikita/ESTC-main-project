#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "pca10059_led.h"
#include "pca10059_button.h"

#define PCA10059_DEVID_SIZE     4

/** @brief Blinking params */
typedef struct
{
    ELedNum     eLed;
    ELedColor   eColor;
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
    if(!psBlinkParam)
        return;

    for(int i = 0; i < psBlinkParam->BlinksCnt; ++i)
    {
        pca10059_LedSetColor(psBlinkParam->eLed, psBlinkParam->eColor);

        nrf_delay_ms(psBlinkParam->BlinkTimems);

        pca10059_LedSetColor(psBlinkParam->eLed, ECOLOR_OFF);

        nrf_delay_ms(psBlinkParam->BlinkTimems);
    }
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
                                                        {ELED_1, ECOLOR_GREEN, 6, 300},
                                                        {ELED_2, ECOLOR_RED, 5, 300},
                                                        {ELED_2, ECOLOR_GREEN, 7, 300},
                                                        {ELED_2, ECOLOR_BLUE, 8, 300}
                                                    };
    ELedColor eColor = msBlinkParams[0].eColor;

    pca10059_leds_init();
    pca10059_button_init();

    while(1)
    {   
        if(BTN_PRESSED == pca10059_GetButtonState())
        {
            if(unTotalTime == msBlinkParams[i].BlinkTimems)
                eColor = ECOLOR_OFF;
            else if (unTotalTime == 2 * msBlinkParams[i].BlinkTimems)
            {
                unTotalTime = 0;
                ++unBlinkCnt;
                if(unBlinkCnt == msBlinkParams[i].BlinksCnt)
                {
                    unBlinkCnt = 0;
                    ++i;
                    if(i == PCA10059_DEVID_SIZE)
                        i = 0;
                }

                eColor = msBlinkParams[i].eColor;
            }

            pca10059_LedSetColor(msBlinkParams[i].eLed, eColor);

            nrf_delay_ms(1);

            ++unTotalTime;
        }
    }
}

