#include "pca10059_button.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include <nrfx_timer.h>
#include "nrfx_systick.h"
#include "app_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#define PCA10059_BUTTON_PIN                 NRF_GPIO_PIN_MAP(1,6)
#define PCA10059_DBLCLICK_HI_TIMEOUT_MS     700
#define PCA10059_DBLCLICK_MIN_TIMEOUT_US    30000

/** @brief Debounce filtering params */
typedef struct
{
    nrfx_systick_state_t    sDebounceSystick;
    uint32_t                unPressCnt;
    bool                    fHIElapsed;
    nrfx_timer_t            sTmr;
}SDebounceParams;


struct 
{
    eBtnState       eBtnIrqState;
    FnButtonHandler fnBtnHandler;               /* Set 0 if not use IRQ */
    void*           pUserData;                  /* user param */    
    SDebounceParams sDebounceParam;
}SIRQParams;

/* ****************************************** */
void pca10059_button_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if(action == NRF_GPIOTE_POLARITY_HITOLO)
    {
        if(SIRQParams.eBtnIrqState == BTN_PRESSED)
        {
            if(SIRQParams.fnBtnHandler)
            {
                SIRQParams.fnBtnHandler(SIRQParams.eBtnIrqState,SIRQParams.pUserData);
            }
        }
        else if (SIRQParams.eBtnIrqState == BTN_DOUBLE_CLICKED)
        {
            SIRQParams.sDebounceParam.unPressCnt++;

            if(SIRQParams.sDebounceParam.unPressCnt == 1)
            {
                nrfx_systick_get(&SIRQParams.sDebounceParam.sDebounceSystick);
                nrfx_timer_enable(&SIRQParams.sDebounceParam.sTmr);
            }

            if(SIRQParams.sDebounceParam.unPressCnt > 1)
            {
                bool LowTimeoutElapsed = false;
                LowTimeoutElapsed = nrfx_systick_test(&SIRQParams.sDebounceParam.sDebounceSystick, PCA10059_DBLCLICK_MIN_TIMEOUT_US);
                if(!LowTimeoutElapsed)
                {
                    nrfx_systick_get(&SIRQParams.sDebounceParam.sDebounceSystick);
                    SIRQParams.sDebounceParam.unPressCnt = 1;
                }
                else
                {
                    if(!SIRQParams.sDebounceParam.fHIElapsed)
                    {
                        nrfx_timer_disable(&SIRQParams.sDebounceParam.sTmr);
                        SIRQParams.sDebounceParam.fHIElapsed = false;
                        SIRQParams.sDebounceParam.unPressCnt = 0;

                        if(SIRQParams.fnBtnHandler)
                        {
                            SIRQParams.fnBtnHandler(SIRQParams.eBtnIrqState,SIRQParams.pUserData);
                        }

                    }
                    else
                    {
                        nrfx_systick_get(&SIRQParams.sDebounceParam.sDebounceSystick);
                        nrfx_timer_enable(&SIRQParams.sDebounceParam.sTmr);
                        SIRQParams.sDebounceParam.fHIElapsed = false;
                        SIRQParams.sDebounceParam.unPressCnt = 1;
                    }
                }
            }
        }
    }


}

void DblClickTimerIrq(nrf_timer_event_t event_type, void* p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
        {
            nrfx_timer_disable(&SIRQParams.sDebounceParam.sTmr);
            SIRQParams.sDebounceParam.fHIElapsed = true;
            break;
        }

        default:
        {
            break;
        }
    }
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

        if(psInitParams->eBtnIrqState == BTN_PRESSED || psInitParams->eBtnIrqState == BTN_DOUBLE_CLICKED)
            sConf.sense = NRF_GPIOTE_POLARITY_HITOLO;
        else
            sConf.sense = NRF_GPIOTE_POLARITY_LOTOHI;

        sConf.pull              = NRF_GPIO_PIN_PULLUP;                   
        sConf.is_watcher        = false;                           
        sConf.hi_accuracy       = false;                         
        sConf.skip_gpio_setup   = false;

        if(psInitParams->eBtnIrqState == BTN_DOUBLE_CLICKED)
        {
            SIRQParams.sDebounceParam.sTmr.p_reg            = NRFX_CONCAT_2(NRF_TIMER, PCA10059_TRM4DBLCLICK);             
            SIRQParams.sDebounceParam.sTmr.instance_id      = NRFX_CONCAT_3(NRFX_TIMER, PCA10059_TRM4DBLCLICK, _INST_IDX); 
            SIRQParams.sDebounceParam.sTmr.cc_channel_count = NRF_TIMER_CC_CHANNEL_COUNT(PCA10059_TRM4DBLCLICK);           

            nrfx_timer_config_t sTmrCfg = NRFX_TIMER_DEFAULT_CONFIG;
            uint32_t unTick = 0;
            uint32_t err_code = NRF_SUCCESS;

            nrfx_systick_init();

            SIRQParams.sDebounceParam.unPressCnt    = 0;
            SIRQParams.sDebounceParam.fHIElapsed    = false;
            

            sTmrCfg.p_context = 0;
            sTmrCfg.frequency = NRF_TIMER_FREQ_1MHz;
            sTmrCfg.bit_width = NRF_TIMER_BIT_WIDTH_32;

            err_code = nrfx_timer_init(&SIRQParams.sDebounceParam.sTmr, &sTmrCfg, DblClickTimerIrq);
            APP_ERROR_CHECK(err_code);

            unTick = nrfx_timer_ms_to_ticks(&SIRQParams.sDebounceParam.sTmr, PCA10059_DBLCLICK_HI_TIMEOUT_MS);

            nrfx_timer_extended_compare(&SIRQParams.sDebounceParam.sTmr, NRF_TIMER_CC_CHANNEL0, unTick, NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
        }

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
     nrfx_gpiote_in_event_disable(PCA10059_BUTTON_PIN);
}