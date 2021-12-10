#include "HSV_to_RGB_Calc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#define RGB_MAX_VAL     255
#define HSV2RGB_COEF    RGB_MAX_VAL/100

#define MAX_VAL_H   360
#define MAX_VAL_S   100
#define MAX_VAL_V   100


void HSVtoRGB_calc(const SHSVCoordinates* psHSV, SRGBCoordinates* psRGB)
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
/* ********************************************************************************* */
void RGBtoHSV_calc(const SRGBCoordinates* psRGB, SHSVCoordinates* psHSV)
{
    uint8_t rgbMin = 0;
    uint8_t rgbMax = 0;

    if(psRGB->R > RGB_MAX_VAL || psRGB->G > RGB_MAX_VAL || psRGB->B > RGB_MAX_VAL)
        return;

    rgbMin = psRGB->R < psRGB->G ? (psRGB->R < psRGB->B ? psRGB->R : psRGB->B) : (psRGB->G  < psRGB->B ? psRGB->G  : psRGB->B);
    rgbMax = psRGB->R > psRGB->G ? (psRGB->R >psRGB->B ? psRGB->R : psRGB->B) : (psRGB->G > psRGB->B? psRGB->G : psRGB->B);

    psHSV->V = rgbMax;
    if (psHSV->V== 0)
    {
        psHSV->H = 0;
        psHSV->S = 0;
        return;
    }

    if (rgbMax == psRGB->R)
        psHSV->H = 0 + 43 * (psRGB->G - psRGB->B) / (rgbMax - rgbMin);
    else if (rgbMax == psRGB->G)
        psHSV->H = 85 + 43 * (psRGB->B - psRGB->R) / (rgbMax - rgbMin);
    else
        psHSV->H = 171 + 43 * (psRGB->R - psRGB->G) / (rgbMax - rgbMin);

    return;
}
/* ********************************************************************************* */
void increment_with_rotate(SHSVCoordinates* psHSV, EHSVParams eParams)
{
    if(!psHSV)
        return;

    switch(eParams)
    {
        case E_PARAM_H:
        {
            ++psHSV->H;

            if(psHSV->H > MAX_VAL_H)
                psHSV->H = 0;

            break;
        }

        case E_PARAM_S:
        {
            ++psHSV->S;

            if(psHSV->S > MAX_VAL_S)
                psHSV->S = 0;

            break;
        }

        case E_PARAM_V: 
        {
            ++psHSV->V;

            if(psHSV->V > MAX_VAL_V)
                psHSV->V = 0;

            break;
        }

        default: return;
    }
}