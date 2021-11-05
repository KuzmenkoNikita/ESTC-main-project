#include "pca10059_button.h"
#include "nrf_gpio.h"

#define PCA10059_BUTTON_PIN         NRF_GPIO_PIN_MAP(1,6)

/* ****************************************** */
void pca10059_button_init(void)
{
    nrf_gpio_cfg_input(PCA10059_BUTTON_PIN, NRF_GPIO_PIN_PULLUP);
}
/* ****************************************** */
int pca10059_GetButtonState(eBtnState* peState)
{
    if(!peState)
        return -1;

    if(nrf_gpio_pin_read(PCA10059_BUTTON_PIN))
        *peState = BTN_RELEASED;
    else
        *peState = BTN_PRESSED;

    return 0;
}