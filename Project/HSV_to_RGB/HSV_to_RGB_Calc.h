#include <stdint.h>

/** @brief HSV Params */
typedef enum
{
    E_PARAM_H,
    E_PARAM_S,
    E_PARAM_V
}EHSVParams;

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
 * @param  pointer to HSV coordinates
 * @param psRGB pointer to RGB coordinates
 */
void HSVtoRGB_calc(const SHSVCoordinates* psHSV, SRGBCoordinates* psRGB);

/**
 * @brief Transform RGB to HSV coordinates
 *
 * @param psRGB pointer to HSV coordinates
 * @param psHSV pointer to RGB coordinates
 */
void RGBtoHSV_calc(const SRGBCoordinates* psRGB, SHSVCoordinates* psHSV);

/**
 * @brief Increment params by one and rotate if it overflows
 *
 * @param eParams params for increment
 * @param psHSV pointer to HSV coordinates
 */
void increment_with_rotate(SHSVCoordinates* psHSV, EHSVParams eParams);


