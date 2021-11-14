#include <stdint.h>

#ifndef PCA10059_LED
#define PCA10059_LED



/** @brief Enumerator used for selecting color */
typedef enum
{
    ECOLOR_OFF,
    ECOLOR_ON
}ELedStete;

/** @brief struct for color components */
typedef struct 
{
    ELedStete   eRedState;
    ELedStete   eGreenState;
    ELedStete   eBlueState;
}SLedColors;


/** @brief Enumerator used for selecting LED */
typedef enum
{
    ELED_1,
    ELED_2,
}ELedNum;

/** @brief Function used for LED initialization 
*/
void pca10059_leds_init(void);

/**
 * @brief Set color LED function
 *
 * @param eLedNum       Specifies the LED
 * @param psColor       pointer to color components struct
 * @return 0 if OK, -1 if LED doesn't support color
 */
int pca10059_LedSetColor(ELedNum eLedNum, SLedColors* psColor);

#endif