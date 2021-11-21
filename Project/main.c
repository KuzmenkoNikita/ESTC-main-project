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
//#include "nrfx_pwm.h"
#include "nrf_delay.h"

#include "nrf_gpio.h"
#include "pca10059_rgb_led.h"

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

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    while(1)
    {
        NRF_LOG_INFO("ERROR HANDLER !!!! ");
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
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
    #if 0
    nrfx_pwm_t sPWMInst = NRFX_PWM_INSTANCE(0);
    nrfx_pwm_config_t const sPWMCfg = 
    {
        .output_pins  = {NRF_GPIO_PIN_MAP(0,6)  | NRFX_PWM_PIN_INVERTED,                    
                      NRFX_PWM_PIN_NOT_USED,                    
                      NRFX_PWM_PIN_NOT_USED,                    
                      NRFX_PWM_PIN_NOT_USED},                  
        .irq_priority = 3,                  
        .base_clock   = NRF_PWM_CLK_1MHz,     
        .count_mode   = NRF_PWM_MODE_UP,    
        .top_value    = 2550,                     
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL, 
        .step_mode    = NRF_PWM_STEP_AUTO
    };
#endif
    sBtnIrq.eBtnIrqState    = BTN_DOUBLE_CLICKED;
    sBtnIrq.fnBtnHandler    = ButtonHandler;
    sBtnIrq.pUserData       = (void*)&fEnable;

    logs_init();

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    //pca10059_BlinkByParams_init(&sInst, LED_PWM_PERIOD_US, msBlinkParams, BLINKING_MAX_PARAM_SIZE);

    pca10059_RGBLed_init();
    pca10059_RGBLed_Set(255, 255, 255);
#if 0
    APP_ERROR_CHECK(nrfx_pwm_init(&sPWMInst, &sPWMCfg, 0));

    static nrf_pwm_values_individual_t sSeqVal;
    sSeqVal.channel_0 = 0;
    sSeqVal.channel_1 = 0;
    sSeqVal.channel_2 = 0;
    sSeqVal.channel_3 = 0; 
    /*
    static uint16_t seq_values[] =
    {
        0x8000,
             0,
        0x8000,
             0,
        0x8000,
             0
    };
    */
   
    nrf_pwm_sequence_t static SSeq =
    {
        .values.p_individual    = &sSeqVal,
        .length                 = NRF_PWM_VALUES_LENGTH(sSeqVal),
        .repeats                = 0,
        .end_delay              = 0
    };
    
    nrfx_pwm_simple_playback(&sPWMInst,&SSeq ,1, NRFX_PWM_FLAG_LOOP);
#endif
    while(1)
    {
       // pca10059_BlinkByParams_process(&sInst, fEnable);

       nrf_delay_ms(5000);
       pca10059_RGBLed_Set(0, 0, 0);
       nrf_delay_ms(5000);
       pca10059_RGBLed_Set(255, 255, 0);

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

