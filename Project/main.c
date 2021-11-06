#include "nrf_delay.h"
#include "pca10059_led.h"
#include <stdint.h>

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
    SBlinkParams msBlinkParams[PCA10059_DEVID_SIZE] = 
                                                    {
                                                        {ELED_1, ECOLOR_GREEN, 6, 300},
                                                        {ELED_2, ECOLOR_RED, 5, 300},
                                                        {ELED_2, ECOLOR_GREEN, 7, 300},
                                                        {ELED_2, ECOLOR_BLUE, 8, 300}
                                                    };


    pca10059_leds_init();

    while(1)
    {
        for(int i = 0; i < PCA10059_DEVID_SIZE; ++i)
        {
            led_blink(&msBlinkParams[i]);
            nrf_delay_ms(1000);
        }
    }
}

