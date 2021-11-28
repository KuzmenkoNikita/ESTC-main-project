#include "LedStateSaver.h"

#define PCA10059_PAGE_SIZE  0x1000
#define PCA10059_PAGES_FOR_SAVE 2

typedef union 
{
    struct 
    {
        uint8_t Red;
        uint8_t Green;
        uint8_t Blue;
        uint8_t CRC;
    };

    uint32_t unVal;

}ULedState;




void LedStateSaver_init(SLedStateSaverInst* psInst, const SLedStateSaverParam* psParam)
{
    uint32_t mPagesStarts[PCA10059_PAGES_FOR_SAVE] = {0};
    uint32_t i = 0;
    uint32_t* pFlashAddr = 0;
    ULedState uState;

    if(!psInst || !psParam)
        return;

    psInst->unFirstPageAddr     = psParam->unFirstPageAddr;
    psInst->unSecondPageAddr    = psParam->unSecondPageAddr;
    psInst->unFlashPageSize     = nrfx_nvmc_flash_page_size_get();

    mPagesStarts[0] = psInst->unFirstPageAddr;
    mPagesStarts[1] = psInst->unSecondPageAddr;

    for(i = 0; i < PCA10059_PAGES_FOR_SAVE; ++i)
    {
        for(pFlashAddr = mPagesStarts[i]; 
            pFlashData < mPagesStarts[i] + psInst->unFlashPageSize; 
            pFlashData += sizeof(uint32_t))
        {
            uState.unVal = *pFlashData;
            if(uState.unVal)
        }
    }




    


}