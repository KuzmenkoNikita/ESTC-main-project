#include "BLE_LedStateSaver.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
/* *********************************************************************************************** */
#define FLASH_PAGE_SIZE 0x1000
#define UNKNOWN_OPERATION_STATE     0xDEADBEEF   
/* *********************************************************************************************** */
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);
/* *********************************************************************************************** */
NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,
};
/* *********************************************************************************************** */
void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if(!p_evt)
        return;
    
    ble_ledsaver_inst* p_saver_inst = (ble_ledsaver_inst*)p_evt->p_param;

    switch(p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            break;
        }

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            p_saver_inst->last_op_state = p_evt->result;

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

    if(p_saver_inst->pages_addrs[0]  > p_saver_inst->pages_addrs[1])
    {
        if(p_saver_inst->pages_addrs[0] != p_saver_inst->pages_addrs[1] + FLASH_PAGE_SIZE)
        {
            return false;
        }

        fstorage.start_addr = p_saver_inst->pages_addrs[1];
        fstorage.end_addr   = p_saver_inst->pages_addrs[0] + FLASH_PAGE_SIZE - 1;
    }
    else if(p_saver_inst->pages_addrs[1]  > p_saver_inst->pages_addrs[0])
    {
        if(p_saver_inst->pages_addrs[1] != p_saver_inst->pages_addrs[0] + FLASH_PAGE_SIZE)
        {
            return false;
        }

        fstorage.start_addr = p_saver_inst->pages_addrs[0];
        fstorage.end_addr   = p_saver_inst->pages_addrs[1] + FLASH_PAGE_SIZE - 1;
    }
    else
    {
        return false;
    }

    if(NRF_SUCCESS != nrf_fstorage_init(&fstorage, &nrf_fstorage_sd, (void*)p_saver_inst))
    {
        return false;
    }

    uint32_t start_pages_data[BLE_LEDSTATESAVER_COUNTOF_PAGES] = {0};
    NRF_LOG_INFO("Start Addr: 0x%x, End addr: 0x%x", fstorage.start_addr, fstorage.end_addr);
    for(uint32_t i = 0; i < BLE_LEDSTATESAVER_COUNTOF_PAGES; ++i)
    {
        
        NRF_LOG_INFO("read addr: 0x%x", p_saver_inst->pages_addrs[i]);

        if(NRF_SUCCESS != nrf_fstorage_read(&fstorage, p_saver_inst->pages_addrs[i], &start_pages_data[i], sizeof(start_pages_data[i])))
        {
            return false;
        }
    }

    //uint32_t active_page_addr = 0;

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
        p_saver_inst->last_op_state = UNKNOWN_OPERATION_STATE;
        NRF_LOG_INFO("Erasing pages...");
        /* if 2 pages is not clear, erease them ... */
        if(NRF_SUCCESS != nrf_fstorage_erase(&fstorage, fstorage.start_addr, 
                            BLE_LEDSTATESAVER_COUNTOF_PAGES, (void*)p_saver_inst))
        {
            return false;
        }

        wait_for_flash_ready(&fstorage);

        while(p_saver_inst->last_op_state == UNKNOWN_OPERATION_STATE);

        if(p_saver_inst->last_op_state != NRF_SUCCESS)
        {
            return false;
        }
        NRF_LOG_INFO("Erased!!");
        //active_page_addr = p_saver_inst->pages_addrs[1];
        p_saver_inst->active_page = 1;
    }

    NRF_LOG_INFO("BLE led saver init OK");

    return true;
}