
#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
/* **************************************************************************************** */
#define ESTC_SERVICE_BLE_OBSERVER_PRIO      3
/* **************************************************************************************** */
static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);
void estc_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
/* **************************************************************************************** */
bool estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;
    ble_uuid_t service_uuid;
    ble_uuid128_t base_uuid = {ESTC_UUID_BASE};

    if(NRF_SUCCESS != sd_ble_uuid_vs_add(&base_uuid, &service->uuid_type))
        return false;

    service_uuid.type   = service->uuid_type;
    service_uuid.uuid   = ESTC_UUID_SERVICE;

    if(NRF_SUCCESS != sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle))
        return false;
    
    if(NRF_SUCCESS != estc_ble_add_characteristics(service))
        return false;

    uint8_t attr_value[]  = "Your User description";
    const uint16_t attr_len   = sizeof(attr_value);
    
    ble_gatts_attr_t    attr;
    memset(&attr, 0, sizeof(attr));
    
    attr.init_len       = attr_len;
    attr.max_len        = attr_len;
    attr.init_offs      = 0;
    attr.p_value        = attr_value;
    attr.p_attr_md      = &attr_md;
    attr.p_uuid         = &attr_uuid;
    
    error_code = sd_ble_gatts_descriptor_add(char_handle, &attr, &p_our_service->descr_handle);
    APP_ERROR_CHECK(error_code); 

    NRF_SDH_BLE_OBSERVER(m_estc_serv_observer, ESTC_SERVICE_BLE_OBSERVER_PRIO, estc_service_on_ble_evt, NULL);

    return true;
}

/**
 * @brief estc module initialization 
 *
 * @param service   pointer to ESTC service instance
 * @return true if OK, false if error
 */
void estc_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        {
            
            break;
        }

        default:
            // No implementation needed.
            break;
    }    
}

/**
 * @brief adding characteristics to the service
 *
 * @param service   pointer to ESTC service instance
 */
static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ble_uuid_t service_uuid;
    service_uuid.type = service->uuid_type;
    service_uuid.uuid = ESTC_UUID_CHAR_1;

    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_ext_props.reliable_wr  = 0;
    char_md.char_ext_props.wr_aux       = 0;

    char_md.char_props.auth_signed_wr   = 0;
    char_md.char_props.broadcast        = 0;
    char_md.char_props.indicate         = 0;
    char_md.char_props.notify           = 0;
    char_md.char_props.read             = 1;
    char_md.char_props.write            = 1;
    char_md.char_props.write_wo_resp    = 0;

    char_md.char_user_desc_max_size     = 10;
    char_md.char_user_desc_size         = 0;

    char_md.p_cccd_md                   = NULL;
    char_md.p_char_pf                   = NULL;
    char_md.p_char_user_desc            = NULL;
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

    attr_char_value.init_len    = sizeof(uint8_t);
    attr_char_value.init_offs   = 0;
    attr_char_value.max_len     = sizeof(uint8_t);
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.p_uuid      = &service_uuid;
    attr_char_value.p_value     = 0;

    return sd_ble_gatts_characteristic_add(service->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &service->char1_handle);
}
