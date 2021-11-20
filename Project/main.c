#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_delay.h"
#include "pca10059_led.h"
#include "pca10059_button.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "nrfx_systick.h"
#include <nrfx_timer.h>
#include "pca10059_led_pwm.h"

#include "pca10059_LedsBlinkByParams.h"


#define LED_PWM_PERIOD_US       1000


typedef struct
{
    nrfx_systick_state_t    sDebounceSystick;
    uint32_t                unPressCnt;
    bool                    fHIElapsed;
    nrfx_timer_t*           psTmr;
}SDebounceParams;


/**
 * @brief Init logs
 */
void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void log_led_color(ELedNum eLed,  bool fRiseColor)
{
    unsigned LedNum = 0;

    switch(eLed)
    {
        case ELED_1: 
        {
            LedNum = 1; 
            break;
        }
        case ELED_2: 
        {
            LedNum = 2;
            break;
        }

        default: 
        {
            NRF_LOG_INFO("Uknown LED coolor \n"); 
            return;
        }
    }

    if(fRiseColor)
    {
        NRF_LOG_INFO("LED %d stated turning on \n", LedNum);
    }
    else
    {
        NRF_LOG_INFO("LED %d stated turning off \n", LedNum); 
    }
}

void assert_nrf_callback(uint16_t line_num, const uint8_t * file_name)
{
    NRF_LOG_INFO(" ASSERT !!!!!"); 
}


void ButtonHandler(eBtnState eState, void* pData)
{
    bool LowTimeoutElapsed = false;

    if(pData)
    {
        NRF_LOG_INFO("BTN PRESSED!!!!!!!!!!!!!!!!!!!!!");
        SDebounceParams* psParams = (SDebounceParams*)pData; 
        psParams->unPressCnt++;

        if(psParams->unPressCnt == 1)
        {
            nrfx_systick_get(&psParams->sDebounceSystick);
            nrfx_timer_enable(psParams->psTmr);
        }

        if(psParams->unPressCnt > 1)
        {
            LowTimeoutElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 30000);
            if(!LowTimeoutElapsed)
            {
                nrfx_systick_get(&psParams->sDebounceSystick);
                psParams->unPressCnt = 1;
            }
            else
            {
                NRF_LOG_INFO("LOW ELAPSED !"); 
                if(!psParams->fHIElapsed)
                {
                    NRF_LOG_INFO("!!! Double click !!!");
                    nrfx_timer_disable(psParams->psTmr);
                    psParams->fHIElapsed = false;
                    psParams->unPressCnt = 0;
                }
                else
                {
                    NRF_LOG_INFO(" HI TMR ELAPSED ");
                    nrfx_systick_get(&psParams->sDebounceSystick);
                    nrfx_timer_enable(psParams->psTmr);
                    psParams->fHIElapsed = false;
                    psParams->unPressCnt = 1;
                }
            }
        }
    }
#if 0
        if(psParams->unPressCnt == 1)
        {
            NRF_LOG_INFO("First Click!");
            nrfx_systick_get(&psParams->sDebounceSystick);
        }

        if(psParams->unPressCnt > 1)
        {
            //bool fHiElapsed = false;
            bool fHiHiElapsed = false;
            
            fHiHiElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 120000);
            if(fHiHiElapsed)
            {
                nrfx_systick_get(&psParams->sDebounceSystick);
                psParams->unPressCnt = 1;
                NRF_LOG_INFO("Too late click");
                return;
            }
            else
            {
                NRF_LOG_INFO("Not Elapsed");
            }

            psParams->fLowElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 30000);
            fHiElapsed = nrfx_systick_test(&psParams->sDebounceSystick, 80000);

            NRF_LOG_INFO(" Hi %d, Low %d", fHiElapsed, psParams->fLowElapsed); 

        
            if (psParams->fLowElapsed && fHiElapsed)
            {
                psParams->unPressCnt = 0;
                NRF_LOG_INFO(" Double CLick!!!!");
            }

            //if (!psParams->fLowElapsed)
            //{
                //--psParams->unPressCnt;
            //}
        }
        else
        {
            NRF_LOG_INFO("ELSE Cnt %u", psParams->unPressCnt);
        }

    } 
#endif
}

void TimerHandler(nrf_timer_event_t event_type, void* p_context)
{
    SDebounceParams* sIrqParams = (SDebounceParams*)p_context;

    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
        {
            NRF_LOG_INFO("Timer IRQ COMPARE 0 !!!!!!");
            nrfx_timer_disable(sIrqParams->psTmr);
            sIrqParams->fHIElapsed = true;
            //nrfx_timer_enable(sTmr);
            break;
        }

        default:
            //Do nothing.
            break;
    }
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    
    SBlinkParams msBlinkParams[BLINKING_MAX_PARAM_SIZE] = 
                                                    {
                                                        {ELED_1, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 6, 500},
                                                        {ELED_2, {ECOLOR_ON, ECOLOR_OFF, ECOLOR_OFF}, 5, 500},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_ON, ECOLOR_OFF}, 7, 500},
                                                        {ELED_2, {ECOLOR_OFF, ECOLOR_OFF, ECOLOR_ON}, 8, 500}
                                                    }; 

    SDebounceParams sBtnIqrParams;
    sBtnIqrParams.unPressCnt = 0;
    SBlinkyInstance sInst;
    SBtnIRQParams sBtnIrq;
    bool fPressed = false;
    uint32_t err_code = NRF_SUCCESS;
    uint32_t unTick = 0;

    nrfx_timer_t sTmr = NRFX_TIMER_INSTANCE(0);
    nrfx_timer_config_t sTmrCfg = NRFX_TIMER_DEFAULT_CONFIG;

    sBtnIqrParams.psTmr = &sTmr;
    sBtnIqrParams.fHIElapsed = false;

    sTmrCfg.p_context = &sBtnIqrParams;
    sTmrCfg.frequency = NRF_TIMER_FREQ_1MHz;
    sTmrCfg.bit_width = NRF_TIMER_BIT_WIDTH_32;


    sBtnIrq.eBtnIrqState = BTN_PRESSED;
    sBtnIrq.fnBtnHandler = ButtonHandler;
    sBtnIrq.pUserData = (void*)&sBtnIqrParams;

    nrfx_systick_init();
    logs_init();

    pca10059_button_init(&sBtnIrq);
    pca10059_button_enable_irq();

    err_code = nrfx_timer_init(&sTmr, &sTmrCfg, TimerHandler);
    APP_ERROR_CHECK(err_code);

    unTick = nrfx_timer_ms_to_ticks(&sTmr, 700);

    nrfx_timer_extended_compare(&sTmr, NRF_TIMER_CC_CHANNEL0, unTick, NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
    //nrfx_timer_extended_compare(&sTmr, NRF_TIMER_CC_CHANNEL1, unTick/2, NRF_TIMER_SHORT_COMPARE1_STOP_MASK, true);

    //nrfx_timer_enable(&ssBtnIqrParams.Tmr);


    pca10059_BlinkByParams_init(&sInst, LED_PWM_PERIOD_US, msBlinkParams, BLINKING_MAX_PARAM_SIZE);

    while(1)
    {
        if (BTN_PRESSED == pca10059_GetButtonState())
            fPressed = true;
        else
            fPressed = false;

        pca10059_BlinkByParams_process(&sInst, fPressed);
        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

