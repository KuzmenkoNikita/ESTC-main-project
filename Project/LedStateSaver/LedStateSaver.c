#include "LedStateSaver.h"
#include <stdbool.h>
#include "CRC8.h"
#include "nrfx_nvmc.h"

#define PCA10059_PAGE_SIZE  0x1000
#define PCA10059_COUNTOF_PARAMS 3
#define PCA10059_CLEAR_FLASH_VAL    0xFFFFFFFF

/** @brief Led color state */
typedef union 
{
    struct 
    {
        unsigned H  : 10;
        unsigned S  : 7;
        unsigned V  : 7;
        unsigned CRC: 8;
    };

    uint32_t unVal;

}ULedState;

/** @brief Calc CRC for the LED color data
*/
uint8_t LedStateSaver_CalcColorsCRC(const SLEDColorState* psColor)
{
    ULedState uState;

    if(!psColor)
        return 0;

    if(psColor->H > 360 || psColor->S > 100 || psColor->V >360)
        return 0;

    uState.H = psColor->H;
    uState.S = psColor->S;
    uState.V = psColor->V;

    return CRC8_calc((uint8_t*)&uState.unVal, PCA10059_COUNTOF_PARAMS);
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
    if(*(uint32_t*)psInst->mFlashPagesAddr[0] == PCA10059_CLEAR_FLASH_VAL)
    {
        ActivePageAddr = psInst->mFlashPagesAddr[1];
        psInst->unActivePageNum = 1;
    }
    else if (*(uint32_t*)psInst->mFlashPagesAddr[1] == PCA10059_CLEAR_FLASH_VAL)
    {
        ActivePageAddr = psInst->mFlashPagesAddr[0];
        psInst->unActivePageNum = 0;
    }
    else
    {
        /* if 2 sectors is not clear, erease them ... */
        for(int i = 0; i < LEDSTATESAVER_COUNTOF_PAGES; ++i)
            if(NRFX_SUCCESS != nrfx_nvmc_page_erase(psInst->mFlashPagesAddr[i]))
                return -1;

        ActivePageAddr = psInst->mFlashPagesAddr[1];
        psInst->unActivePageNum = 1;       
    }

    /* find last data  */
    for(FlashAddr = ActivePageAddr + psInst->unFlashPageSize - sizeof(uint32_t);
        FlashAddr >= ActivePageAddr;
        FlashAddr-=sizeof(uint32_t))
    {
        if(*(uint32_t*)FlashAddr != PCA10059_CLEAR_FLASH_VAL)
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
    SLEDColorState sLesState;

    if(!psInst || !psLedState)
        return -1;

    uState.unVal = *((uint32_t*)psInst->unReadAddr);
    
    if(uState.unVal == PCA10059_CLEAR_FLASH_VAL)
        return -1;

    sLesState.H = uState.H;
    sLesState.S = uState.S;
    sLesState.V = uState.V;

    if(uState.CRC == LedStateSaver_CalcColorsCRC(&sLesState))
    {
        *psLedState = sLesState;
    }
    else
    {
        return -1;
    }

    return 0;
}
/* **************************************************************************************** */
void LedStateSaver_ChangePage(SLedStateSaverInst* psInst)
{
    if(!psInst)
        return;

    psInst->unActivePageNum = (psInst->unActivePageNum + 1) % LEDSTATESAVER_COUNTOF_PAGES;

    psInst->unWriteAddr =  psInst->mFlashPagesAddr[psInst->unActivePageNum];
    return ;
}
/* **************************************************************************************** */
void LedStateSaver_SaveLedState(SLedStateSaverInst* psInst, const SLEDColorState* psLedState)
{
    ULedState uState;
    bool IsNeedErease = false;
    uint32_t unErasedPageAddr = 0;

    if(!psInst || !psLedState)
        return;

    if(psLedState->H > 360 || psLedState->S > 100 || psLedState->V >360)
        return;

    uint32_t unEndPage = psInst->mFlashPagesAddr[psInst->unActivePageNum] + psInst->unFlashPageSize;

    if(psInst->unWriteAddr == unEndPage)
    {
        unErasedPageAddr = psInst->mFlashPagesAddr[psInst->unActivePageNum];
        IsNeedErease = true;
        LedStateSaver_ChangePage(psInst);
        
    }

    uState.CRC = LedStateSaver_CalcColorsCRC(psLedState);

    uState.H = psLedState->H;
    uState.S = psLedState->S;
    uState.V = psLedState->V;

    nrfx_nvmc_word_write(psInst->unWriteAddr, uState.unVal); 

    psInst->unReadAddr = psInst->unWriteAddr;
    psInst->unWriteAddr += sizeof(uint32_t);

    if(IsNeedErease)
    {
        nrfx_nvmc_page_erase(unErasedPageAddr);
    }

}