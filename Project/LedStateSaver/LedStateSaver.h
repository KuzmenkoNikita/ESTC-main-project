#ifndef LED_STATE_SAVER
#define LED_STATE_SAVER

#include <stdint.h>

typedef struct 
{
    uint32_t unFirstPageAddr;
    uint32_t unSecondPageAddr;
}SLedStateSaverParam;

typedef struct 
{
    uint32_t unFirstPageAddr;
    uint32_t unSecondPageAddr;
    uint32_t unFlashPageSize;
}SLedStateSaverInst;

void LedStateSaver_init(SLedStateSaverInst* psInst, const SLedStateSaverParam* psParam);

#endif /* LED_STATE_SAVER */