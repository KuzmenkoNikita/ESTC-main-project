#include "LedStateSaver.h"
#include <stdbool.h>
#include "CRC8.h"
#include "nrfx_nvmc.h"

#define PCA10059_PAGE_SIZE  0x1000
#define PCA10059_COUNTOF_PARAMS 3

/** @brief Led color state */
typedef union 
{
    struct 
    {
        SLEDColorState sColorParams;
        uint8_t CRC;
    };

    uint32_t unVal;

}ULedState;

/** @brief Calc CRC for the LED color data
*/
uint8_t LedStateSaver_CalkColorsCRC(const SLEDColorState* psColor)
{
    if(!psColor)
        return 0;

    /* make temp mass for safety, (align or smth else...) do not use poinet to struct directly */
    uint8_t unTempMas[PCA10059_COUNTOF_PARAMS] = {psColor->Red, psColor->Green, psColor->Blue};

    return CRC8_calc(unTempMas, PCA10059_COUNTOF_PARAMS);
}
/* **************************************************************************************** */
int8_t LedStateSaver_init(SLedStateSaverInst* psInst, const SLedStateSaverParam* psParam)
{
    uint32_t FlashAddr = 0;
    uint32_t ActivePageAddr = 0;

    if(!psInst || !psParam)
        return -1;

    psInst->mFlashPagesAddr[0]  = psParam->unFirstPageAddr;
    psInst->mFlashPagesAddr[1]  = psParam->unSecondPageAddr;
    psInst->unFlashPageSize     = nrfx_nvmc_flash_page_size_get();
    psInst->unReadAddr          = 0;
    psInst->unWriteAddr         = 0;

    /* one sector is always clear */
    if(*(uint32_t*)psInst->mFlashPagesAddr[0] == 0xFFFFFFFF)
    {
        ActivePageAddr = psInst->mFlashPagesAddr[1];
        psInst->unActivePageNum = 1;
    }
    else if (*(uint32_t*)psInst->mFlashPagesAddr[1] == 0xFFFFFFFF)
    {
        ActivePageAddr = psInst->mFlashPagesAddr[0];
        psInst->unActivePageNum = 0;
    }
    else
    {
        /* unexpected behaviour */
        return -1;
    }

    /* find last data  */
    for(FlashAddr = ActivePageAddr + psInst->unFlashPageSize - sizeof(uint32_t);
        FlashAddr >= ActivePageAddr;
        FlashAddr-=sizeof(uint32_t))
    {
        if(*(uint32_t*)FlashAddr != 0xFFFFFFFF)
        {
            break;
        }
    }

    /* if first 4 bytes is ereased */
    if(FlashAddr < ActivePageAddr)
    {
        psInst->unWriteAddr = ActivePageAddr;
        psInst->unReadAddr = ActivePageAddr;
    }
    else 
    {
        psInst->unReadAddr = FlashAddr;
        psInst->unWriteAddr = FlashAddr + sizeof(uint32_t);
    }
    return 0;
}
/* **************************************************************************************** */
int8_t LedStateSaver_GetStateFromFlash(SLedStateSaverInst* psInst, SLEDColorState* psLedState)
{
    ULedState uState;
    if(!psInst || !psLedState)
        return -1;

    uState.unVal = *((uint32_t*)psInst->unReadAddr);
    
    if(uState.unVal == 0xFFFFFFFF)
        return -1;

    if(uState.CRC == LedStateSaver_CalkColorsCRC(&uState.sColorParams))
        *psLedState = uState.sColorParams;
    else
        return -1;

    return 0;
}
/* **************************************************************************************** */
void LedStateSaver_ChangePage(SLedStateSaverInst* psInst)
{
    if(!psInst)
        return;

    switch(psInst->unActivePageNum)
    {
        case 0:
        {
            psInst->unActivePageNum = 1;
            break;
        }

        case 1:
        {
            psInst->unActivePageNum = 0;
            break;
        }

        default: return;
    }

    psInst->unWriteAddr =  psInst->mFlashPagesAddr[psInst->unActivePageNum];
    return ;
}
/* **************************************************************************************** */
void LedStateSaver_SaveLedState(SLedStateSaverInst* psInst, SLEDColorState* psLedState)
{
    ULedState uState;
    bool IsNeedErease = false;
    uint32_t unErasedPageAddr = 0;

    if(!psInst || !psLedState)
        return;

    uint32_t unEndPage = psInst->mFlashPagesAddr[psInst->unActivePageNum] + psInst->unFlashPageSize;

    if(psInst->unWriteAddr == unEndPage)
    {
        unErasedPageAddr = psInst->mFlashPagesAddr[psInst->unActivePageNum];
        IsNeedErease = true;
        LedStateSaver_ChangePage(psInst);
        
    }

    uState.CRC = LedStateSaver_CalkColorsCRC(psLedState);
    uState.sColorParams = *psLedState;

    nrfx_nvmc_word_write(psInst->unWriteAddr, uState.unVal); 

    psInst->unReadAddr = psInst->unWriteAddr;
    psInst->unWriteAddr += sizeof(uint32_t);

    if(IsNeedErease)
    {
        nrfx_nvmc_page_erase(unErasedPageAddr);
    }

}