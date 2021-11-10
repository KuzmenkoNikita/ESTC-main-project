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

#define PCA10059_DEVID_SIZE     4

/** @brief Blinking params */
typedef struct
{
    ELedNum     eLed;
    ELedColor   eColor;
    uint32_t    BlinksCnt;
    uint32_t    BlinkTimems;
}SBlinkParams;


/**
 * @brief Function for blinking led
 * @param eLed Led num for blink
 * @param eColor Led color
 * @param unCnt count of blinks
 */
void led_blink(SBlinkParams* psBlinkParam)
{
    if(!psBlinkParam)
        return;

    for(int i = 0; i < psBlinkParam->BlinksCnt; ++i)
    {
        pca10059_LedSetColor(psBlinkParam->eLed, psBlinkParam->eColor);

        nrf_delay_ms(psBlinkParam->BlinkTimems);

        pca10059_LedSetColor(psBlinkParam->eLed, ECOLOR_OFF);

        nrf_delay_ms(psBlinkParam->BlinkTimems);
    }
}

/**
 * @brief Init logs
 */
void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void log_led_color(ELedNum eLed, ELedColor eColor)
{
    unsigned LedNum = 0;
    char Color[10] = "";
    
    switch(eColor)
    {
        case ECOLOR_OFF:    strcpy(Color, "OFF"); break;
        case ECOLOR_GREEN:  strcpy(Color, "Green"); break;  
        case ECOLOR_RED:    strcpy(Color, "Red"); break;
        case ECOLOR_BLUE:   strcpy(Color, "Blue"); break;
        case ECOLOR_ORANGE: strcpy(Color, "Orange"); break;
        default: NRF_LOG_INFO("Uknown LED coolor \n"); return;
    }

    switch(eLed)
    {
        case ELED_1: LedNum = 1; break;
        case ELED_2: LedNum = 2; break;
        default: NRF_LOG_INFO("Uknown LED coolor \n"); return;
    }

    NRF_LOG_INFO("Led color %s, LED num: %u \n", Color, LedNum);
}


/**
 * @brief Function for application main entry.
 */
int main(void)
{
    unsigned int unTotalTime = 0;
    unsigned int i = 0;
    unsigned int unBlinkCnt = 0;

    SBlinkParams msBlinkParams[PCA10059_DEVID_SIZE] = 
                                                    {
                                                        {ELED_1, ECOLOR_GREEN, 6, 300},
                                                        {ELED_2, ECOLOR_RED, 5, 300},
                                                        {ELED_2, ECOLOR_GREEN, 7, 300},
                                                        {ELED_2, ECOLOR_BLUE, 8, 300}
                                                    };
    ELedColor eColor = msBlinkParams[0].eColor;
    bool ChangeColor = true;

    pca10059_leds_init();
    pca10059_button_init();
    logs_init();

    while(1)
    {   
        if(BTN_PRESSED == pca10059_GetButtonState())
        {
            if(unTotalTime == msBlinkParams[i].BlinkTimems)
            {
                ChangeColor = true;
                eColor = ECOLOR_OFF;
            }
            else if (unTotalTime == 2 * msBlinkParams[i].BlinkTimems)
            {
                unTotalTime = 0;
                ++unBlinkCnt;
                if(unBlinkCnt == msBlinkParams[i].BlinksCnt)
                {
                    unBlinkCnt = 0;
                    ++i;
                    if(i == PCA10059_DEVID_SIZE)
                        i = 0;
                }

                ChangeColor = true;
                eColor = msBlinkParams[i].eColor;
            }

            if(ChangeColor)
            {
                ChangeColor = !ChangeColor; 
                log_led_color(msBlinkParams[i].eLed, eColor);

                pca10059_LedSetColor(msBlinkParams[i].eLed, eColor);
            }

            nrf_delay_ms(1);

            ++unTotalTime;
        }

        LOG_BACKEND_USB_PROCESS();
        NRF_LOG_PROCESS();
    }
}

