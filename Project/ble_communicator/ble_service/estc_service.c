
#include "estc_service.h"
#include <string.h>
#include "app_error.h"
#include "nrf_log.h"

#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
/* **************************************************************************************** */
#define USER_DESC_MAX_SIZE                  20
#define LED_CHARACTERISTIC_MAX_LEN          2
#define LED_CHARACTERISTICS_CNT             3
/* **************************************************************************************** */
typedef struct estc_service
{
    ble_estc_service_t*         p_service;
    uint16_t                    uuid;
    ble_gatts_char_handles_t*   p_char_handle;
    const char*                 user_desc_string;
    bool                        read;
    bool                        write;
    bool                        notify;
    bool                        indicate;
}led_ctrl_char_params;
/* **************************************************************************************** */
static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);
void estc_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
static ret_code_t estc_ble_add_ledctrl_char(led_ctrl_char_params* p_char_params);
static void on_write(ble_estc_service_t *service, ble_evt_t const * p_ble_evt);

/* **************************************************************************************** */
bool estc_ble_service_init(ble_estc_service_t *service, ble_estc_service_info* p_init_info)
{
    ble_uuid_t service_uuid;
    ble_uuid128_t base_uuid = {ESTC_UUID_BASE};

    service->fn_char_write_cb   = p_init_info->fn_char_write_callback;
    service->p_ctx              = p_init_info->p_ctx;
    service->fn_tx_done         = p_init_info->fn_tx_done_callback;

    if(NRF_SUCCESS != sd_ble_uuid_vs_add(&base_uuid, &service->uuid_type))
        return false;

    service_uuid.type   = service->uuid_type;
    service_uuid.uuid   = ESTC_UUID_SERVICE;

    if(NRF_SUCCESS != sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle))
        return false;
    
    if(NRF_SUCCESS != estc_ble_add_characteristics(service))
        return false;

    return true;
}
/** @brief Function for handling the Write event.
 *
 *  @param[in] service    estc service instance
 *  @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_write(ble_estc_service_t *service, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    
    if(p_evt_write->len > LED_CHARACTERISTIC_MAX_LEN)
        return;

    uint32_t data = p_evt_write->data[0] | (p_evt_write->data[1] << 8);
    uint16_t uuid = 0;

    switch(p_evt_write->uuid.uuid)
    {
        case ESTC_UUID_CHAR_LED_H:
        {
            uuid = ESTC_UUID_CHAR_LED_H;
            break;
        }

        case ESTC_UUID_CHAR_LED_S:
        {
            uuid = ESTC_UUID_CHAR_LED_S;
            break;
        }

        case ESTC_UUID_CHAR_LED_V:
        {
            uuid = ESTC_UUID_CHAR_LED_V;
            break;
        }

        default: return;
    }
    
    if(NULL != service->fn_char_write_cb)
    {
        service->fn_char_write_cb(uuid, data, service->p_ctx);
    }
}

void estc_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_estc_service_t* service = (ble_estc_service_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
        {
            on_write(service, p_ble_evt);
            break;
        }

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        {
            service->fn_tx_done(service->p_ctx);
            break;
        }

        default:
            // No implementation needed.
            break;
    }    
}

/**
 * @brief add custom characteristic for led components control
 *
 * @param service           pointer to ESTC service instance
 * @param uuid              char uuid
 * @param p_char_handle     pointer to char handle
 * @param user_desc_string  UTF-8 string with char description
 */
static ret_code_t estc_ble_add_ledctrl_char(led_ctrl_char_params* p_char_params)
{
    if(!p_char_params)
        return NRF_ERROR_SDK_COMMON_ERROR_BASE;

    if(strlen(p_char_params->user_desc_string) > USER_DESC_MAX_SIZE)
        return NRF_ERROR_SDK_COMMON_ERROR_BASE;

    ble_uuid_t service_uuid;
    service_uuid.type = p_char_params->p_service->uuid_type;
    service_uuid.uuid = p_char_params->uuid;

    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_ext_props.reliable_wr  = 0;
    char_md.char_ext_props.wr_aux       = 0;

    char_md.char_props.auth_signed_wr   = 0;
    char_md.char_props.broadcast        = 0;
    char_md.char_props.indicate         = p_char_params->indicate;
    char_md.char_props.notify           = p_char_params->notify;
    char_md.char_props.read             = p_char_params->read;
    char_md.char_props.write            = p_char_params->write;
    char_md.char_props.write_wo_resp    = 0;

    char_md.char_user_desc_max_size     = USER_DESC_MAX_SIZE;
    char_md.char_user_desc_size         = strlen(p_char_params->user_desc_string);

    char_md.p_cccd_md                   = NULL;
    char_md.p_char_pf                   = NULL;
    char_md.p_char_user_desc            = (const uint8_t*)p_char_params->user_desc_string;
    char_md.p_sccd_md                   = NULL;
    char_md.p_user_desc_md              = NULL;
    
    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc            = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth         = 0;
    attr_md.vlen            = 0;
    attr_md.wr_auth         = 0;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    ble_gatts_attr_t attr_char_value = { 0 };

    attr_char_value.init_len    = sizeof(uint16_t);
    attr_char_value.init_offs   = 0;
    attr_char_value.max_len     = sizeof(uint16_t);
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.p_uuid      = &service_uuid;
    attr_char_value.p_value     = 0;

    return sd_ble_gatts_characteristic_add(p_char_params->p_service->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           p_char_params->p_char_handle);    
}                        

/**
 * @brief adding characteristics to the service
 *
 * @param service   pointer to ESTC service instance
 */
static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    if(!service)
        return NRF_ERROR_BASE_NUM;

    led_ctrl_char_params m_led_char_params[LED_CHARACTERISTICS_CNT] = 
    {
        {service, ESTC_UUID_CHAR_LED_V, &service->char_led_v_handle, "Led hue", false, false, true, false},
        {service, ESTC_UUID_CHAR_LED_S, &service->char_led_s_handle, "Led sat", false, false, false, true},
        {service, ESTC_UUID_CHAR_LED_H, &service->char_led_h_handle, "Led val", true, true, false, false},
    };

    ret_code_t err_code = NRF_SUCCESS;
    for(uint8_t i = 0; i < LED_CHARACTERISTICS_CNT; ++i)
    {
        err_code = estc_ble_add_ledctrl_char(&m_led_char_params[i]);
        if(err_code != NRF_SUCCESS)
            return err_code;
    }

    return NRF_SUCCESS;
}

/* **************************************************************************************************** */
bool estc_service_send_char_value(uint16_t conn_handle, ble_estc_service_t *service, 
                                uint16_t char_uuid, uint16_t value, ble_estc_send_type send_method)
{
    ble_gatts_hvx_params_t params;
    uint16_t len = sizeof(value);

    memset(&params, 0, sizeof(params));

    switch(send_method)
    {
        case BLE_ESTC_SEND_BY_NOTIFICATION:
        {
            params.type   = BLE_GATT_HVX_NOTIFICATION;
            break;
        }

        case BLE_ESTC_SEND_BY_INDICATION:
        {
            params.type   = BLE_GATT_HVX_INDICATION;
            break;
        }

        default: return false;
    }

    switch(char_uuid)
    {
        case ESTC_UUID_CHAR_LED_H:
        {
            params.handle = service->char_led_h_handle.value_handle;
            break;
        }   

        case ESTC_UUID_CHAR_LED_S:
        {
            params.handle = service->char_led_s_handle.value_handle;
            break;
        } 

        case ESTC_UUID_CHAR_LED_V:
        {
            params.handle = service->char_led_v_handle.value_handle;
            break;
        } 

        default: return false;
    }

    params.p_data = (const uint8_t*)&value;
    params.p_len  = &len;

    if(NRF_SUCCESS != sd_ble_gatts_hvx(conn_handle, &params))
        return false;

    return true;
}

