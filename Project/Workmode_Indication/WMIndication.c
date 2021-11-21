#include "WMIndication.h"
#include "nrfx_pwm.h"
#include "nrf_gpio.h"
/* ************************************************************************* */
#define PCA10059_WM_LED_PIN             NRF_GPIO_PIN_MAP(0,6)
#define PCA10059_WM_PWMCOEFS_COUNT      2
#define PCA10059_WM_TIMEOUT_TICKS       30000
#define PCA10059_WM_PWM_MAXBLINKCOEF    0x8000
#define PCA10059_WM_PWM_FASTBLINKCOEF   20000
/* ************************************************************************* */
static uint16_t seq_values[PCA10059_WM_PWMCOEFS_COUNT];
/* ************************************************************************* */
EWMTypes eSavedWM; 
/* ************************************************************************* */
void WMIndication_ledPWM_callback(nrfx_pwm_evt_type_t event_type)
{
    if(event_type == NRFX_PWM_EVT_FINISHED)
    {
        unsigned i = 0;

        switch(eSavedWM)
        {
            case EWM_NO_INPUT:
            {
                for(i = 0; i < PCA10059_WM_PWMCOEFS_COUNT; ++i)
                    seq_values[i] = 0;

                break;
            }

            case EWM_TUNING_H:
            {
                seq_values[0] = PCA10059_WM_PWM_MAXBLINKCOEF;
                seq_values[1] = 0;

                break;
            }


            case EWM_TUNING_S:
            {
                seq_values[0] = PCA10059_WM_PWM_FASTBLINKCOEF;
                seq_values[1] = PCA10059_WM_PWM_FASTBLINKCOEF;

                break;
            }

            case EWM_TUNING_V:
            {
                for(i = 0; i < PCA10059_WM_PWMCOEFS_COUNT; ++i)
                    seq_values[i] = PCA10059_WM_PWM_MAXBLINKCOEF;

                break;
            }

            default:
            {
                return;
            }
            
        }
    }
}
/* ************************************************************************* */
int8_t WMIndication_init(void)
{
    nrfx_pwm_t sPWMInst = NRFX_PWM_INSTANCE(WM_INDICATION_PWM_INSTANCE);
    unsigned i = 0;

    nrfx_pwm_config_t const sPWMCfg = 
    {
        .output_pins  = {PCA10059_WM_LED_PIN  | NRFX_PWM_PIN_INVERTED,                    
                        NRFX_PWM_PIN_NOT_USED,                  
                        NRFX_PWM_PIN_NOT_USED,                    
                        NRFX_PWM_PIN_NOT_USED},                  
        .irq_priority = 3,                  
        .base_clock   = NRF_PWM_CLK_125kHz,     
        .count_mode   = NRF_PWM_MODE_UP,    
        .top_value    = PCA10059_WM_TIMEOUT_TICKS,                     
        .load_mode    = NRF_PWM_LOAD_COMMON, 
        .step_mode    = NRF_PWM_STEP_AUTO
    };

    if(NRFX_SUCCESS != nrfx_pwm_init(&sPWMInst, &sPWMCfg, WMIndication_ledPWM_callback))
        return -1;

    nrf_pwm_sequence_t const seq =
    {
        .values.p_common = seq_values,
        .length          = NRF_PWM_VALUES_LENGTH(seq_values),
        .repeats         = 0,
        .end_delay       = 0
    };

    for(i = 0; i < PCA10059_WM_PWMCOEFS_COUNT; ++i)
        seq_values[i] = 0;

    nrfx_pwm_simple_playback(&sPWMInst, &seq, 1, NRFX_PWM_FLAG_LOOP);

    return 0;
}
/* ************************************************************************* */
void WMIndication_SetWM(EWMTypes eWM)
{
    eSavedWM = eWM;
}
/* ************************************************************************* */