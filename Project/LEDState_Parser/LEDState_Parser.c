#include "LEDState_Parser.h"
#include "nrf_ringbuf.h"

#define LED_STATE_PARSER_RINGBUF_SIZE   128

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

    psInstance->mCMDData[psInstance->unCMDBytesCount] = *pData;

    nrf_ringbuf_free(&m_PutData_ringbuf, unGetSize);

    ++psInstance->unCMDBytesCount;

    if(*pData == '\n' || *pData == '\r')
    {
        if(psInstance->unCMDBytesCount > 16)
        {
            if(psInstance->fnCMDError != 0)
            {
                psInstance->fnCMDError(psInstance->pData);
            }
        }
    }


}

