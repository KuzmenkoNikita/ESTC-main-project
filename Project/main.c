#include <stdbool.h>
#include "nrf_delay.h"
#include "pca10059_led.h"
#include "pca10059_button.h"

#define PCA10059_DEVID_SIZE     4

/**
 * @brief Function for blinking led
 * @param eLed Led num for blink
 * @param eColor Led color
 * @param unCnt count of blinks
 */
void led_blink(ELedNum eLed, ELedColor eColor, unsigned int unCnt)
{
    for(int i = 0; i < unCnt; ++i)
    {
        pca10059_LedSetColor(eLed, eColor);

        nrf_delay_ms(300);

        pca10059_LedSetColor(eLed, ECOLOR_OFF);

        nrf_delay_ms(300);
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
    eBtnState BtnState = BTN_UNDEFINED;
    bool OffState = false;
    unsigned int mBlinkParams[PCA10059_DEVID_SIZE][3] =
                                                        {
                                                            {ELED_1, ECOLOR_GREEN, 6},
                                                            {ELED_2, ECOLOR_RED, 5},
                                                            {ELED_2, ECOLOR_GREEN, 7},
                                                            {ELED_2, ECOLOR_BLUE, 8},
                                                        };
    pca10059_leds_init();
    pca10059_button_init();

    while(1)
    {
        pca10059_GetButtonState(&BtnState);
        
        if(BtnState == BTN_PRESSED)
        {
            if(!OffState)
            {
                pca10059_LedSetColor(mBlinkParams[i][0], mBlinkParams[i][1]);

                nrf_delay_ms(1);

                ++unTotalTime;

                if(unTotalTime == 300)
                {
                    unTotalTime = 0; 
                    OffState = true;
                }
            }
            else
            {
                pca10059_LedSetColor(mBlinkParams[i][0], ECOLOR_OFF); 

                nrf_delay_ms(1);

                ++unTotalTime;

                if(unTotalTime == 300)
                {
                    unTotalTime = 0; 
                    OffState = false;
                    unBlinkCnt++;
                    if(mBlinkParams[i][2] == unBlinkCnt)
                    {
                        unBlinkCnt = 0;
                        ++i;
                    }
                }
            }
        }
    }
}

