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

#define HELP_CMD_MASS                   "help"
#define RGB_SET_CMD_MASS                "RGB"

NRF_RINGBUF_DEF(m_PutData_ringbuf, LED_STATE_PARSER_RINGBUF_SIZE);

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
bool LEDStateParser_GetFirstIntVal(uint8_t* pMas, uint32_t unSize, uint16_t* punIntVal)
{
    char szVal[PARAM_MAX_NUMERALS] = "";
    
    if(!pMas || !unSize)
        return false;
    
    for(int i = 0; i < unSize; ++i)
    {
        if(i > PARAM_MAX_NUMERALS)
            return false;

        if(!LEDStateParser_IsNum(pMas[i]))
            return false;

        if(pMas[i] == ' ')
            break;
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
            if(psInstance->unCMDBytesCount == MIN_CDM_LEN || 
                (psInstance->unCMDBytesCount == MIN_CDM_LEN + 1 && 
                psInstance->mCMDData[MIN_CDM_LEN] == ' '))
            {
                if(!memcmp(psInstance->mCMDData, HELP_CMD_MASS, MIN_CDM_LEN))
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
                if(!memcmp(psInstance->mCMDData, RGB_SET_CMD_MASS, 1))
                {
                    if(psInstance->mCMDData[MIN_CDM_LEN] != ' ')
                    {
                        if(psInstance->fnCMDError != 0)
                            psInstance->fnCMDError(psInstance->pData);
                    }
                    else
                    {
                        int32_t RemainBytes = psInstance->unCMDBytesCount - MIN_CDM_LEN;
                        for(uint32_t i = 0; i < RemainBytes; ++i)
                        {
                            if(psInstance->mCMDData[MIN_CDM_LEN]+i  >= '0' &&  
                            psInstance->mCMDData[MIN_CDM_LEN]+i >= '9')
                            {

                            }
                        }
                    }
                }
            }

        }

        psInstance->unCMDBytesCount = 0;
    }

    nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);
}

