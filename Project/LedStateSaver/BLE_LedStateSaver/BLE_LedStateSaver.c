#include "BLE_LedStateSaver.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "CRC8.h"
#include "color_data_structures.h"

#include <stddef.h>

/* *********************************************************************************************** */
#define FLASH_PAGE_SIZE             0x1000
#define START_ERASE_INPROGRESS      0xDEADBEEF

/* *********************************************************************************************** */
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
static inline bool is_page_contains(uint32_t page_start_addr, uint32_t addr);

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
static nrf_fstorage_t* p_storages[BLE_LEDSTATESAVER_COUNTOF_PAGES] = {&fstorage_page_0, &fstorage_page_1};
/* *********************************************************************************************** */
static inline bool is_page_contains(uint32_t page_start_addr, uint32_t addr)
{
    return ((addr >= page_start_addr) && (addr < page_start_addr + FLASH_PAGE_SIZE));
}
/* *********************************************************************************************** */
void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if(!p_evt)
        return;
    
    ble_ledsaver_inst* p_saver_inst = (ble_ledsaver_inst*)p_evt->p_param;
    if(!p_saver_inst)
        return;

    switch(p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            ble_ledsaver_state saver_state = p_evt->result == NRF_SUCCESS ? BLE_LEDSAVER_WRITE_SUCCESSFUL : BLE_LEDSAVER_WRITE_ERROR;

            if(p_saver_inst->state_changed_callback != NULL)
            {
                p_saver_inst->state_changed_callback(saver_state, p_saver_inst->p_data);
                return;
            }

            break;
        }

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            if(p_saver_inst->last_op_state[0] == START_ERASE_INPROGRESS 
                || p_saver_inst->last_op_state[1] == START_ERASE_INPROGRESS)
            {
                
                if(p_evt->result != NRF_SUCCESS)
                {
                    if(p_saver_inst->state_changed_callback != NULL)
                    {
                        p_saver_inst->state_changed_callback(BLE_LEDSAVER_INIT_ERROR, p_saver_inst->p_data);
                        return;
                    }
                }
                else
                {               
                    if(is_page_contains(p_saver_inst->pages_addrs[0], p_evt->addr))
                    {
                        p_saver_inst->last_op_state[0] = NRF_SUCCESS;
                    }
                    else if (is_page_contains(p_saver_inst->pages_addrs[1], p_evt->addr))
                    {
                        p_saver_inst->last_op_state[1] = NRF_SUCCESS;
                    }
                    else
                    {
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
            return;
        }
    }
    
}
/* *********************************************************************************************** */
bool led_state_saver_init(ble_ledsaver_inst* p_saver_inst, const ble_ledsaver_init* p_init_params)
{
    if(!p_init_params || !p_saver_inst)
    {
        return false;
    }

    p_saver_inst->pages_addrs[0]    = p_init_params->first_page;
    p_saver_inst->pages_addrs[1]    = p_init_params->second_page;
    p_saver_inst->read_addr         = 0;
    p_saver_inst->write_addr        = 0;

    p_saver_inst->state_changed_callback    = p_init_params->state_changed_callback;
    p_saver_inst->p_data                    = p_init_params->p_data;

    uint32_t start_pages_data[BLE_LEDSTATESAVER_COUNTOF_PAGES] = {0};

    for(uint32_t i = 0; i < BLE_LEDSTATESAVER_COUNTOF_PAGES; ++i)
    {
        p_storages[i]->start_addr   = p_saver_inst->pages_addrs[i];
        p_storages[i]->end_addr     = p_saver_inst->pages_addrs[i] + FLASH_PAGE_SIZE - 1;

        if(NRF_SUCCESS != nrf_fstorage_init(p_storages[i], &nrf_fstorage_sd, (void*)p_saver_inst))
        {
            return false;
        }

        if(NRF_SUCCESS != nrf_fstorage_read(p_storages[i], p_saver_inst->pages_addrs[i], &start_pages_data[i], sizeof(start_pages_data[i])))
        {
            return false;
        }
    }

    uint32_t active_page_addr = 0;

    if(start_pages_data[0] == 0xFFFFFFFF)
    {
        active_page_addr = p_saver_inst->pages_addrs[1];
        p_saver_inst->active_page = 1;
    }
    else if (start_pages_data[1] == 0xFFFFFFFF)
    {
        active_page_addr = p_saver_inst->pages_addrs[0];
        p_saver_inst->active_page = 0;
    }
    else
    {
        /* if 2 pages is not clear, erease them ... */

        for(uint32_t i = 0; i < BLE_LEDSTATESAVER_COUNTOF_PAGES; ++i)
        {
            p_saver_inst->last_op_state[i] = START_ERASE_INPROGRESS;

            if(NRF_SUCCESS != nrf_fstorage_erase(p_storages[i], p_storages[i]->start_addr, 1, (void*)p_saver_inst))
            {
                return false;
            }
        }

        active_page_addr = p_saver_inst->pages_addrs[1];
        p_saver_inst->active_page = 1;
        p_saver_inst->write_addr = active_page_addr;
        p_saver_inst->read_addr = active_page_addr;
        return true;
    }

    uint32_t flash_addr = 0;

    /* find last data  */
    for(flash_addr = active_page_addr + FLASH_PAGE_SIZE - sizeof(uint32_t); 
        flash_addr >= active_page_addr; 
        flash_addr-=sizeof(uint32_t))
    {
        uint32_t read_value = 0;

        if(NRF_SUCCESS != nrf_fstorage_read(p_storages[p_saver_inst->active_page], flash_addr, &read_value, sizeof(read_value)))
        {
            return false;
        }

        if(read_value != 0xFFFFFFFF)
            break;
    }

    /* if first 4 bytes is ereased */
    if(flash_addr < active_page_addr)
    {
        p_saver_inst->write_addr = active_page_addr;
        p_saver_inst->read_addr = active_page_addr;
    }
    else 
    {
        p_saver_inst->read_addr = flash_addr;
        p_saver_inst->write_addr = flash_addr + sizeof(uint32_t);
    }

    if(p_saver_inst->state_changed_callback != NULL)
    {
        p_saver_inst->state_changed_callback(BLE_LEDSAVER_INIT_SUCCESSFUL, p_saver_inst->p_data);
    }

    return true;
}
/* *********************************************************************************************** */
bool led_state_saver_get_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led)
{
    if(!p_saver_inst || !p_hsv_led)
    {
        return false;
    }

    uint32_t flash_data = 0;


    if(NRF_SUCCESS != nrf_fstorage_read(p_storages[p_saver_inst->active_page], p_saver_inst->read_addr, &flash_data, sizeof(flash_data)))
    {
        return false;
    }

    if(flash_data == 0xFFFFFFFF)
    {
        return false;
    }

    uint8_t crc = CRC8_calc((uint8_t*)&flash_data, 3);

    if(crc != get_crc_from_flashdata(flash_data))
    {
        return false;
    }

    p_hsv_led->H = get_led_hue_from_flashdata(flash_data);
    p_hsv_led->S = get_led_sat_from_flashdata(flash_data);
    p_hsv_led->V = get_led_val_from_flashdata(flash_data);

    return true;
}
/* *********************************************************************************************** */
bool led_state_saver_save_state(ble_ledsaver_inst* p_saver_inst, SHSVCoordinates* p_hsv_led)
{
    if(!p_saver_inst || !p_hsv_led)
        return false;

    uint32_t end_of_page = p_saver_inst->pages_addrs[p_saver_inst->active_page] + FLASH_PAGE_SIZE;

    uint32_t page_to_erase = 0;
    bool is_need_erase = false;

    if(p_saver_inst->write_addr == end_of_page)
    {
        page_to_erase = p_saver_inst->pages_addrs[p_saver_inst->active_page];
        is_need_erase = true;

        p_saver_inst->active_page = (p_saver_inst->active_page + 1) % BLE_LEDSTATESAVER_COUNTOF_PAGES;
        p_saver_inst->write_addr =  p_saver_inst->pages_addrs[p_saver_inst->active_page];
    }

    uint32_t data_to_save = 0;

    data_to_save = set_led_hue_to_flashdata(data_to_save, p_hsv_led->H);
    data_to_save = set_led_sat_to_flashdata(data_to_save, p_hsv_led->S);
    data_to_save = set_led_val_to_flashdata(data_to_save, p_hsv_led->V);

    uint8_t crc = CRC8_calc((uint8_t*)&data_to_save, 3);

    data_to_save = set_crc_to_flashdata(data_to_save, crc);

    p_saver_inst->data_to_write = data_to_save;

    if(NRF_SUCCESS != nrf_fstorage_write(p_storages[p_saver_inst->active_page],  p_saver_inst->write_addr, 
                                        &p_saver_inst->data_to_write, sizeof(p_saver_inst->data_to_write), (void*)p_saver_inst))
    {
        return false;
    }

    p_saver_inst->read_addr = p_saver_inst->write_addr;
    p_saver_inst->write_addr += sizeof(uint32_t);

    if(is_need_erase)
    {
        if(NRF_SUCCESS != nrf_fstorage_erase(p_storages[p_saver_inst->active_page], page_to_erase, 1, (void*)p_saver_inst))
        {
            return false;
        }
    }

    return true;

}