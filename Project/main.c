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
    eBtnState BtnState = BTN_UNDEFINED;
    unsigned int mBlinkParams[PCA10059_DEVID_SIZE][3] =
                                                        {
                                                            {ELED_1, ECOLOR_GREEN, 6},
                                                            {ELED_2, ECOLOR_RED, 5},
                                                            {ELED_2, ECOLOR_GREEN, 7},
                                                            {ELED_2, ECOLOR_BLUE, 8},
                                                        };
    /*
    unsigned int mDevID[PCA10059_DEVID_SIZE] = {6,5,7,8};
    unsigned int mLedsNum[PCA10059_DEVID_SIZE] = {ELED_1, ELED_2, ELED_2, ELED_2};
    unsigned int mColors[PCA10059_DEVID_SIZE] = {ECOLOR_GREEN, ECOLOR_RED, ECOLOR_GREEN, ECOLOR_BLUE};
    */
    pca10059_leds_init();
    pca10059_button_init();

    while(1)
    {
        pca10059_GetButtonState(&BtnState);
        
        if(BtnState == BTN_PRESSED)
        {
            for(int i = 0; i < PCA10059_DEVID_SIZE; ++i)
            {
                led_blink(mBlinkParams[i][0], mBlinkParams[i][1], mBlinkParams[i][2]);
                nrf_delay_ms(1000);
            }
        }
    }
}

