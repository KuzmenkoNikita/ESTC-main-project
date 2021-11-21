#include "pca10059_rgb_led.h"
#include "nrfx_pwm.h"
#include "nrf_gpio.h"

/* ******************************************************************** */
#define PCA10059_RGBLED_GREEN_PIN     NRF_GPIO_PIN_MAP(1,9) 
#define PCA10059_RGBLED_RED_PIN       NRF_GPIO_PIN_MAP(0,8) 
#define PCA10059_RGBLED_BLUE_PIN      NRF_GPIO_PIN_MAP(0,12)
/* ******************************************************************** */
#define PCA10059_RGBLED_PWM_MAX_VAL     255
#define PCA10059_RGBLED_STEP_TICK       10
#define PCA10059_PWM_PERIOD_TICKS       PCA10059_RGBLED_PWM_MAX_VAL * PCA10059_RGBLED_STEP_TICK
/* ******************************************************************** */

nrf_pwm_values_individual_t static sSeqVal;
nrf_pwm_sequence_t static SSeq =
{
    .values.p_individual    = &sSeqVal,
    .length                 = NRF_PWM_VALUES_LENGTH(sSeqVal),
    .repeats                = 0,
    .end_delay              = 0
};
/* ******************************************************************** */
struct 
{
    uint16_t Red;
    uint16_t Green;
    uint16_t Blue;
}sColorComponents;
/* ******************************************************************** */
void pca10059_RGBledPWM_callback(nrfx_pwm_evt_type_t event_type)
{
    if(event_type == NRFX_PWM_EVT_FINISHED)
    {
        sSeqVal.channel_0 = sColorComponents.Red;
        sSeqVal.channel_1 = sColorComponents.Green;
        sSeqVal.channel_2 = sColorComponents.Blue;
    }
}
/* ******************************************************************** */
int32_t pca10059_RGBLed_init(void)
{
    nrfx_pwm_t sPWMInst = NRFX_PWM_INSTANCE(PCA10059_PWM_INST_NUM);
    nrfx_pwm_config_t const sPWMCfg = 
    {
        .output_pins  = {PCA10059_RGBLED_RED_PIN  | NRFX_PWM_PIN_INVERTED,                    
                        PCA10059_RGBLED_GREEN_PIN | NRFX_PWM_PIN_INVERTED,                  
                        PCA10059_RGBLED_BLUE_PIN | NRFX_PWM_PIN_INVERTED,                    
                        NRFX_PWM_PIN_NOT_USED},                  
        .irq_priority = 3,                  
        .base_clock   = NRF_PWM_CLK_1MHz,     
        .count_mode   = NRF_PWM_MODE_UP,    
        .top_value    = PCA10059_PWM_PERIOD_TICKS,                     
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL, 
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    if(NRFX_SUCCESS != nrfx_pwm_init(&sPWMInst, &sPWMCfg, pca10059_RGBledPWM_callback))
        return -1;

    sSeqVal.channel_0 = 0;
    sSeqVal.channel_1 = 0;
    sSeqVal.channel_2 = 0;
    sSeqVal.channel_3 = 0; 

    sColorComponents.Red    = 0;
    sColorComponents.Green  = 0;
    sColorComponents.Blue   = 0;

    nrfx_pwm_simple_playback(&sPWMInst, &SSeq ,1, NRFX_PWM_FLAG_LOOP);

    return 0;
}
/* ******************************************************************** */
void pca10059_RGBLed_Set(uint8_t Red, uint8_t Green, uint8_t Blue)
{
    sColorComponents.Red    = Red * PCA10059_RGBLED_STEP_TICK;
    sColorComponents.Green  = Green * PCA10059_RGBLED_STEP_TICK;
    sColorComponents.Blue   = Blue * PCA10059_RGBLED_STEP_TICK;
}
/* ******************************************************************** */