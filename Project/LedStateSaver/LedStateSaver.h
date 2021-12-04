#ifndef LED_STATE_SAVER
#define LED_STATE_SAVER

#include <stdint.h>

#define LEDSTATESAVER_COUNTOF_PAGES 2

/** @brief Color params */
typedef struct 
{
    uint16_t    H;
    uint8_t     S;
    uint8_t     V;
}SLEDColorState;

/** @brief module init params. This module requires 2 flash pages*/
typedef struct 
{
    uint32_t unFirstPageAddr;   
    uint32_t unSecondPageAddr;
}SLedStateSaverParam;

/** @brief module instance */
typedef struct 
{
    uint32_t mFlashPagesAddr[LEDSTATESAVER_COUNTOF_PAGES];
    uint32_t unFlashPageSize;
    uint32_t unWriteAddr;
    uint32_t unReadAddr;
    uint32_t unActivePageNum;
}SLedStateSaverInst;

/**
 * @brief init module
 *
 * @param psInst    pointer to module instance struct
 * @param psParam   pointer to init param struct
 * @return 0 if OK, -1 if Error
 */
int8_t LedStateSaver_init(SLedStateSaverInst* psInst, const SLedStateSaverParam* psParam);

/**
 * @brief Retrieves led state from flash
 *
 * @param psInst    pointer to module instance struct
 * @param psLedState   pointer to LED state struct
 * @return 0 if OK, -1 if Error or if flash pages are clear
 */
int8_t LedStateSaver_GetStateFromFlash(SLedStateSaverInst* psInst, SLEDColorState* psLedState);

/**
 * @brief Save led state to flash memory
 *
 * @param psInst    pointer to module instance struct
 * @param psLedState   pointer to LED state struct
 */
void LedStateSaver_SaveLedState(SLedStateSaverInst* psInst, const SLEDColorState* psLedState);

#endif /* LED_STATE_SAVER */