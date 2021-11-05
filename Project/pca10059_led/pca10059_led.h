#include <stdint.h>

/** @brief Enumerator used for selecting color */
typedef enum
{
    ECOLOR_OFF,
    ECOLOR_GREEN,
    ECOLOR_RED,
    ECOLOR_BLUE,
    ECOLOR_ORANGE,
}ELedColor;

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
 * @param eColor        LED color
 * @return 0 if OK, -1 if LED doesn't support color
 */
int pca10059_LedSetColor(ELedNum eLedNum, ELedColor eColor);