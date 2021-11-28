#ifndef WM_INDICATION
#define WM_INDICATION

#include <stdint.h>

/** @brief PWM instance for WM indication module */
#define WM_INDICATION_PWM_INSTANCE  1

/** @brief workmodes type */
typedef enum
{
    EWM_NO_INPUT,
    EWM_TUNING_H,
    EWM_TUNING_S,
    EWM_TUNING_V,
    EWM_COUNT_WM
}EWMTypes;

/**
 * @brief Init workmode indication modile
 * @return 0 if OK, -1 if Error
 */
int8_t WMIndication_init(void);

/**
 * @brief Set wormode
 *
 * @param eWM workmode
 */
void WMIndication_SetWM(EWMTypes eWM);

#endif /* WM_INDICATION */