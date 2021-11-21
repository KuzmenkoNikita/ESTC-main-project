#include <stdint.h>

/** @brief Structure with HSV coordincates */
typedef struct 
{
    uint16_t    H;
    uint8_t     S;
    uint8_t     V;
}SHSVCoordinates;

/** @brief Structure with RGB coordincates */
typedef struct 
{
    uint8_t     R;
    uint8_t     G;
    uint8_t     B;
}SRGBCoordinates;

/**
 * @brief Transform HSV to RGB coordinates
 *
 * @param psHSV pointer to HSV coordinates
 * @param psRGB pointer to RGB coordinates
 */
void HSVtoRGB_calc(SHSVCoordinates* psHSV, SRGBCoordinates* psRGB);
