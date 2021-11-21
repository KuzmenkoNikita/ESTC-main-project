#include "HSV_to_RGB_Calc.h"

#define HSV2RGB_COEF    255/100

void HSVtoRGB_calc(SHSVCoordinates* psHSV, SRGBCoordinates* psRGB)
{
    uint32_t unHi = 0;
    uint32_t VMin = 0;
    uint32_t a = 0;
    uint32_t Vinc = 0;
    uint32_t Vdet = 0;

    if(!psHSV || !psRGB)
        return;

    if(psHSV->H > 360 || psHSV->S > 100 || psHSV->V > 100)
        return;

    unHi = (psHSV->H / 60) % 6;
    VMin = ((100 - psHSV->S) * psHSV->V) / 100;
    a = (psHSV->V - VMin) * (psHSV->H % 60) / 60;
    Vinc = VMin + a;
    Vdet = psHSV->V - a;

    switch(unHi)
    {
        case 0:
        {
            psRGB->R = psHSV->V * HSV2RGB_COEF;
            psRGB->G = Vinc * HSV2RGB_COEF;
            psRGB->B = VMin * HSV2RGB_COEF;

            break;
        }

        case 1:
        {
            psRGB->R = Vdet * HSV2RGB_COEF;
            psRGB->G = psHSV->V * HSV2RGB_COEF;
            psRGB->B = VMin * HSV2RGB_COEF;

            break;
        }

        case 2:
        {
            psRGB->R = VMin * HSV2RGB_COEF;
            psRGB->G = psHSV->V * HSV2RGB_COEF;
            psRGB->B = Vinc * HSV2RGB_COEF;

            break;
        }

        case 3:
        {
            psRGB->R = VMin * HSV2RGB_COEF;
            psRGB->G = Vdet * HSV2RGB_COEF;
            psRGB->B = psHSV->V * HSV2RGB_COEF;

            break;
        }

        case 4:
        {
            psRGB->R = Vinc * HSV2RGB_COEF;
            psRGB->G = VMin * HSV2RGB_COEF;
            psRGB->B = psHSV->V * HSV2RGB_COEF;

            break;
        }

        case 5:
        {
            psRGB->R = psHSV->V * HSV2RGB_COEF;
            psRGB->G = VMin * HSV2RGB_COEF;
            psRGB->B = Vdet * HSV2RGB_COEF;

            break;
        }

        default: return;
    }
}