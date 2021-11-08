#include "pca10059_button.h"
#include "nrf_gpio.h"

#define PCA10059_BUTTON_PIN         NRF_GPIO_PIN_MAP(1,6)

/* ****************************************** */
void pca10059_button_init(void)
{
    nrf_gpio_cfg_input(PCA10059_BUTTON_PIN, NRF_GPIO_PIN_PULLUP);
}
/* ****************************************** */
eBtnState pca10059_GetButtonState(void)
{
    if(nrf_gpio_pin_read(PCA10059_BUTTON_PIN))
        return BTN_RELEASED;
    else
        return BTN_PRESSED;
}