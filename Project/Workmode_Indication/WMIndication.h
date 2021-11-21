#ifndef WM_INDICATION
#define WM_INDICATION

#include <stdint.h>

typedef enum
{
    EWM_NO_INPUT,
    EWM_TUNING_H,
    EWM_TUNING_S,
    EWM_TUNING_V
}EWMTypes;

int8_t WMIndication_init(void);

void WMIndication_SetWM(EWMTypes eWM);

#endif /* WM_INDICATION */