#include "LEDState_Parser.h"
#include "nrf_ringbuf.h"
#include <string.h>
#include <stdbool.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#define LED_STATE_PARSER_RINGBUF_SIZE   128
#define MAX_CDM_LEN                     15
#define MIN_CDM_LEN                     4 /* help */
#define PARAM_MAX_NUMERALS              3

#define RGB_MAX_PARAMS_VALUE            255
#define HSV_MAX_PARAM_H                 360
#define HSV_MAX_PARAM_S                 100
#define HSV_MAX_PARAM_V                 100

#define HELP_CMD_MASS                   "help"
#define RGB_SET_CMD_MASS                "RGB "
#define HSV_SET_CMD_MASS                "HSV "

NRF_RINGBUF_DEF(m_PutData_ringbuf, LED_STATE_PARSER_RINGBUF_SIZE);

typedef enum
{
    LEDSTATEPARSER_HELP_REQUEST,
    LEDSTATEPARSER_SET_RGB,
    LEDSTATEPARSER_SET_HSV
}ELEDStParserCMDType;

typedef struct 
{
    uint16_t unParam1;
    uint16_t unParam2;
    uint16_t unParam3;
}SLEDStParserCMDParams;


/* ***************************************************************************************** */
uint32_t LEDStateParser_init(SLEDStateParserInst* psInstance, SLEDStateParserInfo* psInit)
{
    if(!psInstance || !psInit)
        return -1;

    psInstance->fnCMDError      = psInit->fnCMDError;
    psInstance->fnHelpRequest   = psInit->fnHelpRequest;
    psInstance->fnSetState      = psInit->fnSetState;
    psInstance->pData           = psInit->pData;
    psInstance->unCMDBytesCount = 0;

    nrf_ringbuf_init(&m_PutData_ringbuf);

    return 0; 
}
/* ***************************************************************************************** */
uint32_t LEDStateParser_PutByte(SLEDStateParserInst* psInstance, char Byte)
{
    if(!psInstance)
        return 0;

    size_t  unSize = 1;
    uint8_t* pPutByte;

    if(NRF_SUCCESS == nrf_ringbuf_alloc(&m_PutData_ringbuf, &pPutByte, &unSize, true))
    {
        if(!unSize)
            return -1;

        *pPutByte = Byte;
        if(NRF_SUCCESS != nrf_ringbuf_put(&m_PutData_ringbuf, unSize))
            return -1;
    }
    else
    {
        return -1;
    }

    return 0;
}
/* ***************************************************************************************** */
bool LEDStateParser_IsNum(uint8_t Byte)
{
    if(Byte >= '0' && Byte <= '9')
        return true;
    else
        return false;
}
/* ***************************************************************************************** */
bool LEDStateParser_GetFirstIntVal(const uint8_t* pMas, uint32_t unMassSize, 
                                    uint16_t* punIntVal, uint32_t* pTakenBytes)
{
    char szVal[PARAM_MAX_NUMERALS] = "";
    
    if(!pMas || !unMassSize || !pTakenBytes)
        return false;
    
    for(int i = 0; i < unMassSize; ++i)
    {
        if(i > PARAM_MAX_NUMERALS - 1)
            return false;

        if(!LEDStateParser_IsNum(pMas[i]))
            return false;

        if(pMas[i] == ' ')
            break;
        
        szVal[i] = pMas[i];
        (*pTakenBytes)++;
    }

    *punIntVal = atoi(szVal);
    return true;
}

/* ***************************************************************************************** */
bool LEDStateParser_ParseParams(const uint8_t* pCMD, uint32_t unBytesInMas, 
                                SLEDStParserCMDParams* psParams)
{
    uint32_t unTakenBytes = 0;
    uint32_t unTotalTakenBytes = 0;

    if(!pCMD || !unBytesInMas)
        return false;

    /* TODO: maybe make it with loop... */
    if(!LEDStateParser_GetFirstIntVal(pCMD, unBytesInMas, &psParams->unParam1, &unTakenBytes))
        return false;

    unTotalTakenBytes += unTakenBytes;

    /* for instance: "255_0_0" we have 4 bytes after 255 */
    if(unBytesInMas - unTotalTakenBytes  < 4)
        return false;

    if(*(pCMD+unTotalTakenBytes+1) != ' ')
        return false;

    unTotalTakenBytes++;

    if(!LEDStateParser_GetFirstIntVal(pCMD + unTotalTakenBytes, unBytesInMas - unTotalTakenBytes, 
                                        &psParams->unParam2, &unTakenBytes))
    {
        return false;
    }

    unTotalTakenBytes += unTakenBytes;

    /* for instance: "255_0_0" we have 2 bytes after 255_0 */
    if(unBytesInMas - unTotalTakenBytes  < 2)
        return false;

    if(*(pCMD+unTotalTakenBytes+1) != ' ')
        return false;

    unTotalTakenBytes++;

    if(!LEDStateParser_GetFirstIntVal(pCMD + unTotalTakenBytes, unBytesInMas - unTotalTakenBytes, 
                                        &psParams->unParam3, &unTakenBytes))
    {
        return false;
    }

    unTotalTakenBytes += unTakenBytes;

    if(unBytesInMas - unTotalTakenBytes != 0)
    {
        if(unBytesInMas - unTotalTakenBytes == 1 && (*(pCMD + unTotalTakenBytes + 1) == ' '))
            return true;
        else
            return false;
    }
    else
    {
        return true;
    }
}

/* ***************************************************************************************** */
bool LEDStateParser_ParseCMDType(ELEDStParserCMDType* peCMDType, const uint8_t* pData, size_t unDataSize)
{
    if(!peCMDType || !pData || unDataSize < MIN_CDM_LEN)
        return false;

    /* TODO: make "for" */
    if(!memcmp(pData, HELP_CMD_MASS, MIN_CDM_LEN))
        *peCMDType = LEDSTATEPARSER_HELP_REQUEST;
    else if (!memcmp(pData, RGB_SET_CMD_MASS, MIN_CDM_LEN))
        *peCMDType = LEDSTATEPARSER_SET_RGB;
    else if (!memcmp(pData, HSV_SET_CMD_MASS, MIN_CDM_LEN))
         *peCMDType = LEDSTATEPARSER_SET_HSV;
    else
        return false;

    return true;                 
}
/* ***************************************************************************************** */
bool LEDStateParser_CheckParams(ELEDStParserCMDType eCMD, SLEDStParserCMDParams* psParams)
{
    if(!psParams)
        return false;

    switch(eCMD)
    {
        case LEDSTATEPARSER_SET_RGB:
        {
            if(psParams->unParam1 > RGB_MAX_PARAMS_VALUE ||
            psParams->unParam2 > RGB_MAX_PARAMS_VALUE   ||
            psParams->unParam2 > RGB_MAX_PARAMS_VALUE)
            {
                return false;
            }

            break;
        }

        case LEDSTATEPARSER_SET_HSV:
        {
            if(psParams->unParam1 > HSV_MAX_PARAM_H ||
            psParams->unParam2 > HSV_MAX_PARAM_S   ||
            psParams->unParam2 > HSV_MAX_PARAM_V)
            {
                return false;
            }
            
            break;
        }

        default: return false;
    }

    return true;
}
/* ***************************************************************************************** */
void LEDStateParser_ExecLEDState(SLEDStateParserInst* psInstance, ELEDStParserCMDType eCMD,
                                SLEDStParserCMDParams* psParams)
{
    ULEDStateParams uParams;
    ETypeParams eParam;
    if(!psInstance || !psParams)
        return;

    switch(eCMD)
    {
        case LEDSTATEPARSER_SET_RGB:
        {
            eParam = ELEDPARSER_RGB;
            uParams.sRGB.R = psParams->unParam1;
            uParams.sRGB.G = psParams->unParam2;
            uParams.sRGB.B = psParams->unParam3;
            break;
        }

        case LEDSTATEPARSER_SET_HSV:
        {
            eParam = ELEDPARSER_RGB;
            uParams.sHSV.H = psParams->unParam1;
            uParams.sHSV.S = psParams->unParam2;
            uParams.sHSV.V = psParams->unParam3;

            break;
        }

        default: return;
    }

    if(psInstance->fnSetState != 0)
        psInstance->fnSetState(&uParams, eParam, psInstance->pData);
}
/* ***************************************************************************************** */
void LEDStateParser_ProcessSetLed(SLEDStateParserInst* psInstance, ELEDStParserCMDType eCMDType)
{
    SLEDStParserCMDParams psParams;
    if(!psInstance)
        return;
    
    if(!LEDStateParser_ParseParams(psInstance->mCMDData + MIN_CDM_LEN, 
                                    psInstance->unCMDBytesCount - MIN_CDM_LEN, 
                                    &psParams))
    {
        if(psInstance->fnCMDError != 0)
            psInstance->fnCMDError(psInstance->pData);
    }
    else
    {
        if(!LEDStateParser_CheckParams(eCMDType, &psParams))
        {
            if(psInstance->fnCMDError != 0)
                psInstance->fnCMDError(psInstance->pData);
        }
        else
        {
            LEDStateParser_ExecLEDState(psInstance, eCMDType, &psParams);
        }
    }    
}
/* ***************************************************************************************** */
void LEDStateParser_ProcessCMD(SLEDStateParserInst* psInstance)
{
    ELEDStParserCMDType eCMDType;
    if(!psInstance)
        return;
            
    if(!LEDStateParser_ParseCMDType(&eCMDType, psInstance->mCMDData, MIN_CDM_LEN))
    {
        if(psInstance->fnCMDError != 0)
            psInstance->fnCMDError(psInstance->pData);
    }
    else
    {
        if(eCMDType == LEDSTATEPARSER_HELP_REQUEST)
        {
            if(psInstance->unCMDBytesCount == MIN_CDM_LEN || 
            (psInstance->unCMDBytesCount == MIN_CDM_LEN + 1 && 
            psInstance->mCMDData[MIN_CDM_LEN] == ' '))
            {
                if(psInstance->fnHelpRequest != 0)
                    psInstance->fnHelpRequest(psInstance->pData);
            }
            else
            {
                if(psInstance->fnCMDError != 0)
                    psInstance->fnCMDError(psInstance->pData);
            }
        }
        else
        {
            LEDStateParser_ProcessSetLed(psInstance, eCMDType);
        }
    }    
}
/* ***************************************************************************************** */
void LEDStateParser_Process(SLEDStateParserInst* psInstance)
{
    if(!psInstance)
        return;

    size_t unGetSize = 1;
    uint8_t* pData;

    if(NRF_SUCCESS != nrf_ringbuf_get(&m_PutData_ringbuf, &pData, &unGetSize, true))
        return;

    if(!unGetSize)
        return;

    if(psInstance->unCMDBytesCount == 20)
        psInstance->unCMDBytesCount = 0;

    if(psInstance->unCMDBytesCount == 0 && *pData == ' ')
    {
        nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);
        return;
    }

    if(psInstance->unCMDBytesCount != 0 && 
        psInstance->mCMDData[psInstance->unCMDBytesCount - 1] == ' ' &&
        *pData == ' ')
    {
        nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);
        return;
    }

    if(*pData != '\n' && *pData != '\r' && *pData != '\b')
    {
        psInstance->mCMDData[psInstance->unCMDBytesCount] = *pData;

        nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);

        ++psInstance->unCMDBytesCount;
    }
    else
    {
        nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);
    }

    if(*pData == '\n' || *pData == '\r')
    {
        NRF_LOG_INFO("Countof bytes: %u \n", psInstance->unCMDBytesCount);

        if(psInstance->unCMDBytesCount > MAX_CDM_LEN || psInstance->unCMDBytesCount <  MIN_CDM_LEN)
        {
            if(psInstance->fnCMDError != 0)
                psInstance->fnCMDError(psInstance->pData);
        }
        else
        {
            LEDStateParser_ProcessCMD(psInstance);
        }

        psInstance->unCMDBytesCount = 0;
    }

    nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);
}

