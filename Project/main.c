#include "nrf_delay.h"
#include "pca10059_led.h"

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
    unsigned int mDevID[PCA10059_DEVID_SIZE] = {6,5,7,8};
    unsigned int mLedsNum[PCA10059_DEVID_SIZE] = {ELED_1, ELED_2, ELED_2, ELED_2};
    unsigned int mColors[PCA10059_DEVID_SIZE] = {ECOLOR_GREEN, ECOLOR_RED, ECOLOR_GREEN, ECOLOR_BLUE};

    pca10059_leds_init();

    while(1)
    {
        for(int i = 0; i < PCA10059_DEVID_SIZE; ++i)
        {
            led_blink(mLedsNum[i], mColors[i], mDevID[i]);
            nrf_delay_ms(1000);
        }
    }
}

