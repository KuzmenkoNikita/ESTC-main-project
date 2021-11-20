#include <stdbool.h>
#include <stdint.h>
#include "pca10059_button.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "pca10059_LedsBlinkByParams.h"
#include "nrfx_pwm.h"

#include "nrf_gpio.h"

#define LED_PWM_PERIOD_US 1000

/**
 * @brief Init logs
 */
void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


void ButtonHandler(eBtnState eState, void* pData)
{
    NRF_LOG_INFO("!!! Double click !!! ");

    if(!pData)
        return;

    bool* pfEnState = (bool*)pData;
    *pfEnState = !(*pfEnState);
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    /*
    SBlinkParams msBlinkParams[BLINKING_MAX_PARAM_SIZE] = 
                                                    {
                                                        {ELED_1, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 6, 500},
                                                        {ELED_2, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 5, 500},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 7, 500},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_OFF, ECOLOR_ON}, 8, 500}
                                                    }; */

   // SBlinkyInstance sInst;
    SBtnIRQParams sBtnIrq;
    bool fEnable = false;
    nrfx_pwm_t sPWMInst;
    nrfx_pwm_config_t const sPWMCfg = 
    {
        .output_pins  = {NRF_GPIO_PIN_MAP(0,6) | NRFX_PWM_PIN_INVERTED,                    
                      NRFX_PWM_PIN_NOT_USED,                    
                      NRFX_PWM_PIN_NOT_USED,                    
                      NRFX_PWM_PIN_NOT_USED},                  
        .irq_priority = 3,                  
        .base_clock   = NRF_PWM_CLK_125kHz,     
        .count_mode   = NRF_PWM_MODE_UP,    
        .top_value    = 10000,                     
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL, 
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    sBtnIrq.eBtnIrqState    = BTN_DOUBLE_CLICKED;
    sBtnIrq.fnBtnHandler    = ButtonHandler;
    sBtnIrq.pUserData       = (void*)&fEnable;

    logs_init();

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    //pca10059_BlinkByParams_init(&sInst, LED_PWM_PERIOD_US, msBlinkParams, BLINKING_MAX_PARAM_SIZE);



    APP_ERROR_CHECK(nrfx_pwm_init(&sPWMInst, &sPWMCfg, 0));

    static uint16_t /*const*/ seq_values[] =
    {
        0x8000,
             0,
        0x8000,
             0,
        0x8000,
             0
    };

    nrf_pwm_sequence_t const SSeq =
    {
        .values.p_common = seq_values,
        .length          = NRF_PWM_VALUES_LENGTH(seq_values),
        .repeats         = 4,
        .end_delay       = 0
    };

    nrfx_pwm_simple_playback(&sPWMInst,&SSeq ,1, NRFX_PWM_FLAG_LOOP);

    while(1)
    {
       // pca10059_BlinkByParams_process(&sInst, fEnable);
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

