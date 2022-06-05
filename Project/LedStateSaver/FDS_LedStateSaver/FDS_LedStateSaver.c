#include "FDS_LedStateSaver.h"
#include "color_data_structures.h"
#include "fds.h"
#include "CRC8.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
/* ****************************************************** */
#define LEDSTATE_FILE_ID     0x1111
#define LEDSTATE_REC_KEY     0x2222
/* ****************************************************** */
/** @brief module instance */
typedef struct 
{
    fds_ledsaver_state_changed  fn_state_changed;
    void*                       p_data;
    uint32_t                    write_data;
    fds_record_desc_t           record_desc;    
}fds_ledsaver_inst;
/* ****************************************************** */
static fds_ledsaver_inst module_inst;
/* ****************************************************** */
/** @brief fds callback*/
void ledstatesaver_fds_callback(fds_evt_t const * p_evt)
{
    fds_ledsaver_state state = FDS_LEDSAVER_STATE_UNDEFINED;

    switch(p_evt->id)
    {
        case FDS_EVT_INIT:
        {
            if(p_evt->result != NRF_SUCCESS)
            {
                NRF_LOG_INFO("ledstatesaver_fds_callback: init OK");
                state = FDS_LEDSAVER_INIT_ERROR;
            }
            else
            {
                NRF_LOG_INFO("ledstatesaver_fds_callback: init ERROR");
                state = FDS_LEDSAVER_INIT_COMPLETE;
            }

            break;
        }

        case FDS_EVT_WRITE:
        case FDS_EVT_UPDATE:
        {
            if(p_evt->result != NRF_SUCCESS)
            {
                NRF_LOG_INFO("ledstatesaver_fds_callback: write OK");
                state = FDS_LEDSAVER_WRITE_ERROR;
            }
            else
            {
                NRF_LOG_INFO("ledstatesaver_fds_callback: Update or Write complete");
                state = FDS_LEDSAVER_WRITE_COMPLETE;
            }

            break;
        }

        default:
        {
            NRF_LOG_INFO("ledstatesaver_fds_callback: unknown event ID");
            break;
        }
    }

    if(module_inst.fn_state_changed != NULL)
    {
        module_inst.fn_state_changed(state, module_inst.p_data);
    }
}

/* *********************************************************************************************** */
bool fds_led_state_saver_init(const fds_ledsaver_init* p_init_params)
{
    if(!p_init_params)
    {
        return false;
    }


    if(NRF_SUCCESS != fds_register(ledstatesaver_fds_callback))
    {
        return false;
    }

    module_inst.fn_state_changed  =   p_init_params->fn_state_changed;
    module_inst.p_data            =   p_init_params->p_data;

    if(NRF_SUCCESS != fds_init())
    {
        return false;
    }

    return true;
}
/* *********************************************************************************************** */
bool fds_led_state_saver_get_state(SHSVCoordinates* p_hsv_led)
{
    if(!p_hsv_led)
    {
        return false;
    }

	fds_find_token_t    find_token ={0};
    fds_flash_record_t  flash_record;

    ret_code_t result = fds_record_find(LEDSTATE_FILE_ID, LEDSTATE_REC_KEY, &module_inst.record_desc, &find_token);

    if(NRF_SUCCESS != result)
    {
        if(result == FDS_ERR_NOT_FOUND)
        {
            fds_record_t      first_record;

            first_record.file_id    = LEDSTATE_FILE_ID;
            first_record.key        = LEDSTATE_REC_KEY;

            first_record.data.length_words  = 1;
            first_record.data.p_data        = &module_inst.write_data;

            if(NRF_SUCCESS != fds_record_write(&module_inst.record_desc, &first_record))
            {
                return false;
            }

            p_hsv_led->H = 0;
            p_hsv_led->S = 0;
            p_hsv_led->V = 0;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if(NRF_SUCCESS != fds_record_open(&module_inst.record_desc, &flash_record))
		{
			return false;		
		}

        uint32_t flash_data = *(uint32_t*)(flash_record.p_data);

        NRF_LOG_INFO("fds_led_state_saver_get_state: readed data: 0x%x", flash_data);

        if(NRF_SUCCESS != fds_record_close(&module_inst.record_desc))
        {
            return false;
        }

        uint8_t calculated_crc = CRC8_calc((uint8_t*)&flash_data, 3);
        uint8_t readed_crc = get_crc_from_flashdata(flash_data);

        if(calculated_crc != readed_crc)
        {
            return false;
        }

        p_hsv_led->H = get_led_hue_from_flashdata(flash_data);
        p_hsv_led->S = get_led_sat_from_flashdata(flash_data);
        p_hsv_led->V = get_led_val_from_flashdata(flash_data);

    }

    return true;
}
/* *********************************************************************************************** */
bool fds_led_state_saver_save_state(const SHSVCoordinates* p_hsv_led)
{
    if(!p_hsv_led)
    {
        return false;
    }

    module_inst.write_data = 0;

    module_inst.write_data = set_led_hue_to_flashdata(module_inst.write_data, p_hsv_led->H);
    module_inst.write_data = set_led_sat_to_flashdata(module_inst.write_data, p_hsv_led->S);
    module_inst.write_data = set_led_val_to_flashdata(module_inst.write_data, p_hsv_led->V);

    uint8_t crc = CRC8_calc((uint8_t*)&module_inst.write_data, 3);

    module_inst.write_data = set_crc_to_flashdata(module_inst.write_data, crc);

    fds_record_t record;

    record.file_id  = LEDSTATE_FILE_ID;
    record.key      = LEDSTATE_REC_KEY;

    record.data.length_words    = 1;
    record.data.p_data          = &module_inst.write_data;

    if(NRF_SUCCESS != fds_record_update(&module_inst.record_desc, &record))
    {
        return false;
    }

    return true;
}