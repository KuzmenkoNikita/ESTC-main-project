#include "BLE_LedStateSaver.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
/* *********************************************************************************************** */
#define FLASH_PAGE_SIZE 0x1000
#define START_ERASE_INPROGRESS     0xDEADBEEF   
/* *********************************************************************************************** */
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);
/* *********************************************************************************************** */
NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage_page_0) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,
};

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage_page_1) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,
};
/* *********************************************************************************************** */
static inline bool is_page_contains(uint32_t page_start_addr, uint32_t addr)
{
    return ((addr >= page_start_addr) && (addr < page_start_addr + FLASH_PAGE_SIZE));
}
/* *********************************************************************************************** */
void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    NRF_LOG_INFO("!!! fstorage_evt_handler !!!");

    if(!p_evt)
        return;
    
    ble_ledsaver_inst* p_saver_inst = (ble_ledsaver_inst*)p_evt->p_param;
    if(!p_saver_inst)
        return;

    switch(p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("fstorage_evt_handler: NRF_FSTORAGE_EVT_WRITE_RESULT");
            break;
        }

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("fstorage_evt_handler: NRF_FSTORAGE_EVT_ERASE_RESULT");

            
            if(p_saver_inst->last_op_state[0] == START_ERASE_INPROGRESS 
                || p_saver_inst->last_op_state[1] == START_ERASE_INPROGRESS)
            {
                NRF_LOG_INFO("fstorage_evt_handler: init in progress");
                
                if(p_evt->result != NRF_SUCCESS)
                {
                    NRF_LOG_INFO("fstorage_evt_handler: bad erase");
                    if(p_saver_inst->state_changed_callback != NULL)
                    {
                        p_saver_inst->state_changed_callback(BLE_LEDSAVER_INIT_ERROR, p_saver_inst->p_data);
                        return;
                    }
                }
                else
                {
                    NRF_LOG_INFO("good erase");
                    
                    if(is_page_contains(p_saver_inst->pages_addrs[0], p_evt->addr))
                    {
                        p_saver_inst->last_op_state[0] = NRF_SUCCESS;
                        NRF_LOG_INFO("Page 0 erase success");
                    }
                    else if (is_page_contains(p_saver_inst->pages_addrs[1], p_evt->addr))
                    {
                        p_saver_inst->last_op_state[1] = NRF_SUCCESS;
                        NRF_LOG_INFO("Page 1 erase success");
                    }
                    else
                    {
                        NRF_LOG_INFO("fstorage_evt_handler: unexpected flash addr");
                        return;
                    }
                    
                    if((p_saver_inst->last_op_state[0] == NRF_SUCCESS) && 
                        (p_saver_inst->last_op_state[1]) == NRF_SUCCESS)
                    {
                        if(p_saver_inst->state_changed_callback != NULL)
                        {
                            p_saver_inst->state_changed_callback(BLE_LEDSAVER_INIT_SUCCESSFUL, p_saver_inst->p_data);
                            return;
                        }
                    }
                    
                }
                
            }
            
            break;
        }

        default:
        {
            NRF_LOG_INFO("fstorage_evt_handler: unexpected event id");
            return;
        }
    }
    
}
/* *********************************************************************************************** */
void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        (void) sd_app_evt_wait();
    }
}
/* *********************************************************************************************** */
bool led_state_saver_init(ble_ledsaver_inst* p_saver_inst, const ble_ledsaver_init* p_init_params)
{
    if(!p_init_params || !p_saver_inst)
    {
        return false;
    }

    NRF_LOG_INFO("Entered func");

    p_saver_inst->pages_addrs[0]    = p_init_params->first_page;
    p_saver_inst->pages_addrs[1]    = p_init_params->second_page;
    p_saver_inst->page_size         = FLASH_PAGE_SIZE;
    p_saver_inst->read_addr         = 0;
    p_saver_inst->write_addr        = 0;

    p_saver_inst->state_changed_callback    = p_init_params->state_changed_callback;
    p_saver_inst->p_data                    = p_init_params->p_data;

    nrf_fstorage_t* p_storages[BLE_LEDSTATESAVER_COUNTOF_PAGES] = {&fstorage_page_0, &fstorage_page_1};
    uint32_t start_pages_data[BLE_LEDSTATESAVER_COUNTOF_PAGES] = {0};

    for(uint32_t i = 0; i < BLE_LEDSTATESAVER_COUNTOF_PAGES; ++i)
    {
        p_storages[i]->start_addr   = p_saver_inst->pages_addrs[i];
        p_storages[i]->end_addr     = p_saver_inst->pages_addrs[i] + FLASH_PAGE_SIZE - 1;

        if(NRF_SUCCESS != nrf_fstorage_init(p_storages[i], &nrf_fstorage_sd, (void*)p_saver_inst))
        {
            NRF_LOG_INFO("nrf_fstorage_init %u error", i);
            return false;
        }

        NRF_LOG_INFO("fstorage_page %u : Start Addr: 0x%x, End addr: 0x%x", i, p_storages[i]->start_addr, p_storages[i]->end_addr);
        NRF_LOG_INFO("read addr: 0x%x", p_saver_inst->pages_addrs[i]);

        if(NRF_SUCCESS != nrf_fstorage_read(p_storages[i], p_saver_inst->pages_addrs[i], &start_pages_data[i], sizeof(start_pages_data[i])))
        {
            NRF_LOG_INFO("Read page %u error", i);
            return false;
        }
        NRF_LOG_INFO("Readed data: 0x%x", start_pages_data[i]);
    }

    NRF_LOG_INFO("Analyse active page...");

#if 0
    uint32_t test_write = 0;
    nrf_fstorage_write(p_storages[0],  p_saver_inst->pages_addrs[0], &test_write, 4, NULL);

    uint32_t test_write1 = 0;
    nrf_fstorage_write(p_storages[1],  p_saver_inst->pages_addrs[1], &test_write1, 4, NULL);
#endif
    if(start_pages_data[0] == 0xFFFFFFFF)
    {
        //active_page_addr = p_saver_inst->pages_addrs[1];
        NRF_LOG_INFO("active page 1");
        p_saver_inst->active_page = 1;
    }
    else if (start_pages_data[1] == 0xFFFFFFFF)
    {
        //active_page_addr = p_saver_inst->pages_addrs[0];
        p_saver_inst->active_page = 0;
        NRF_LOG_INFO("active page 0");
    }
    else
    {
        NRF_LOG_INFO("Erasing pages...");

        /* if 2 pages is not clear, erease them ... */
        #if 1
        for(uint32_t i = 0; i < BLE_LEDSTATESAVER_COUNTOF_PAGES; ++i)
        {
            p_saver_inst->last_op_state[i] = START_ERASE_INPROGRESS;

            if(NRF_SUCCESS != nrf_fstorage_erase(p_storages[i], p_storages[i]->start_addr, 1, (void*)p_saver_inst))
            {
                NRF_LOG_INFO("Erase page %u error...", i);
                return false;
            }
        }
        #endif
        //active_page_addr = p_saver_inst->pages_addrs[1];
        p_saver_inst->active_page = 1;
        //p_saver_inst->write_addr = active_page_addr;
        //p_saver_inst->read_addr = active_page_addr;
        NRF_LOG_INFO("BLE led saver init OK with erase");
        return true;
    }

    NRF_LOG_INFO("BLE led saver init OK");

    return true;
}