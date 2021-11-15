#include "pca10059_button.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

#define PCA10059_BUTTON_PIN         NRF_GPIO_PIN_MAP(1,6)

struct 
{
    eBtnState       eBtnIrqState;
    FnButtonHandler fnBtnHandler;               /* Set 0 if not use IRQ */
    void*           pUserData;                  /* user param */    
}SIRQParams;

/* ****************************************** */
void pca10059_button_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if(SIRQParams.fnBtnHandler)
        SIRQParams.fnBtnHandler(SIRQParams.eBtnIrqState,SIRQParams.pUserData);
}

/* ****************************************** */
void pca10059_button_init(SBtnIRQParams* psInitParams)
{
    nrf_gpio_cfg_input(PCA10059_BUTTON_PIN, NRF_GPIO_PIN_PULLUP);

    if(psInitParams && psInitParams->fnBtnHandler)
    {
        nrfx_gpiote_in_config_t sConf;

        SIRQParams.fnBtnHandler = psInitParams->fnBtnHandler;
        SIRQParams.pUserData    = psInitParams->pUserData;
        SIRQParams.eBtnIrqState = psInitParams->eBtnIrqState;

        nrfx_gpiote_init();

        if(psInitParams->eBtnIrqState == BTN_PRESSED)
            sConf.sense = NRF_GPIOTE_POLARITY_HITOLO;
        else
            sConf.sense = NRF_GPIOTE_POLARITY_LOTOHI;

        sConf.pull = NRF_GPIO_PIN_NOPULL;                   
        sConf.is_watcher = false;                           
        sConf.hi_accuracy = false;                         
        sConf.skip_gpio_setup = false;

        nrfx_gpiote_in_init	(PCA10059_BUTTON_PIN, &sConf, pca10059_button_handler);
    }
}
/* ****************************************** */
eBtnState pca10059_GetButtonState(void)
{
    if(nrf_gpio_pin_read(PCA10059_BUTTON_PIN))
        return BTN_RELEASED;
    else
        return BTN_PRESSED;
}
/* ****************************************** */
void pca10059_button_enable_irq(void)
{
     nrfx_gpiote_in_event_enable(PCA10059_BUTTON_PIN, true);
}
/* ****************************************** */
void pca10059_button_disable_irq(void)
{
     nrfx_gpiote_in_event_enable(PCA10059_BUTTON_PIN, false);
}