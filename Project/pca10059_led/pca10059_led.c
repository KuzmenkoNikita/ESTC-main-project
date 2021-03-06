#include "nrf_gpio.h"
#include "pca10059_led.h"

#define PCA10059_LED1_GREEN_PIN     NRF_GPIO_PIN_MAP(0,6)
#define PCA10059_LED2_GREEN_PIN     NRF_GPIO_PIN_MAP(1,9) 
#define PCA10059_LED2_RED_PIN       NRF_GPIO_PIN_MAP(0,8) 
#define PCA10059_LED2_BLUE_PIN      NRF_GPIO_PIN_MAP(0,12) 

/* *********************************************** */
void pca10059_leds_init(void)
{
    /* LED 1 */
    nrf_gpio_cfg_output(PCA10059_LED1_GREEN_PIN);

    /* LED 2 */
    nrf_gpio_cfg_output(PCA10059_LED2_GREEN_PIN);
    nrf_gpio_cfg_output(PCA10059_LED2_RED_PIN);
    nrf_gpio_cfg_output(PCA10059_LED2_BLUE_PIN);

    nrf_gpio_pin_write(PCA10059_LED1_GREEN_PIN, 1);
    nrf_gpio_pin_write(PCA10059_LED2_GREEN_PIN, 1);
    nrf_gpio_pin_write(PCA10059_LED2_RED_PIN, 1);
    nrf_gpio_pin_write(PCA10059_LED2_BLUE_PIN, 1);
}

/* *********************************************** */
int pca10059_LedSetColor(ELedNum eLedNum, SLedColors* psColor)
{
    unsigned unGrenState = 0;
    unsigned unRedState = 0;
    unsigned unBlueState = 0;

    if(!psColor)
        return -1;

    unBlueState = psColor->eBlueState   == ECOLOR_ON ? 0 : 1;
    unGrenState = psColor->eGreenState  == ECOLOR_ON ? 0 : 1;
    unRedState  = psColor->eRedState    == ECOLOR_ON ? 0 : 1;

    if(eLedNum == ELED_1)
    {
        nrf_gpio_pin_write(PCA10059_LED1_GREEN_PIN, unGrenState);
    }
    else if(eLedNum == ELED_2)
    {
        nrf_gpio_pin_write(PCA10059_LED2_GREEN_PIN, unGrenState);
        nrf_gpio_pin_write(PCA10059_LED2_RED_PIN, unRedState);
        nrf_gpio_pin_write(PCA10059_LED2_BLUE_PIN, unBlueState);
    }
    else
        return -1;

    return 0;
}