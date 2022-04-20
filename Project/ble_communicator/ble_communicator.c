#include "ble_communicator.h"
#include "app_error.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "ble_advertising.h"
#include "estc_service.h"
#include "ble_conn_params.h"
#include "app_timer.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"
/* ******************************************************** */
#define DEVICE_NAME                     "ESTC"                                  /**< Name of device. Will be included in the advertising data. */
#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */
/* ******************************************************** */
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */
/* ********************************************************************************** */
#define APP_ADV_INTERVAL                300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define APP_ADV_DURATION                18000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
/* ********************************************************************************** */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */
/* ********************************************************************************** */
static bool ble_stack_init(void);
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static bool gap_params_init(void);
static bool gatt_init(void);
static bool advertising_init(void);
static void on_adv_evt(ble_adv_evt_t ble_adv_evt);
static bool services_init(ble_communicator_t* p_ctx);
static bool conn_params_init(void);
static void conn_params_error_handler(uint32_t nrf_error);
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
static bool advertising_start(void);
/* ********************************************************************************** */
BLE_ESTC_SERVICE_DEF(m_estc_service);
NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */
/* ********************************************************************************** */
static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    {ESTC_UUID_SERVICE, BLE_UUID_TYPE_BLE}
};
/* ********************************************************************************** */
/** @brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
        {
            NRF_LOG_INFO("Disconnected.");
            break;
        }

        case BLE_GAP_EVT_CONNECTED:
        {
            NRF_LOG_INFO("Connected.");

            break;
        }

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);

            break;
        } 

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
        {
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
        }

        default:
            // No implementation needed.
            break;
    }
}

/** @brief Function for initializing the BLE stack.
 *
 *  @details Initializes the SoftDevice and the BLE event interrupt.
 */
static bool ble_stack_init(void)
{
    if(NRF_SUCCESS != nrf_sdh_enable_request())
        return false;

    uint32_t ram_start = 0;
    if(NRF_SUCCESS != nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start))
        return false;

    if(NRF_SUCCESS != nrf_sdh_ble_enable(&ram_start))
        return false;

    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

    return true;
}

/** @brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static bool gap_params_init(void)
{
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    if(NRF_SUCCESS != sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME)))
    {
        return false;
    }

	if(NRF_SUCCESS != sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN))
        return false;

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    if(NRF_SUCCESS != sd_ble_gap_ppcp_set(&gap_conn_params))
        return false;

    return true;
}

/** @brief Function for initializing the GATT module.
 */
static bool gatt_init(void)
{
    if(NRF_SUCCESS != nrf_ble_gatt_init(&m_gatt, NULL))
        return false;

    return true;
}

/** @brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");

            break;

        default:
            break;
    }
}

void estc_service_write_cb (uint16_t char_uuid, uint32_t write_val, void* p_ctx)
{
    
    ble_communicator_t* p_communicator = (ble_communicator_t*)p_ctx;

    ble_led_components color_component;

    switch(char_uuid)
    {
        case ESTC_UUID_CHAR_LED_H:
        {
            color_component = BLE_LED_COMPONENT_H;

            if(write_val > 360)
                return;

            break;
        }
        case ESTC_UUID_CHAR_LED_S:
        {
            color_component = BLE_LED_COMPONENT_S;

            if(write_val > 255)
                return;

            break;
        }

        case ESTC_UUID_CHAR_LED_V:
        {
            color_component = BLE_LED_COMPONENT_V;

            if(write_val > 255)
                return;

            break;
        }

        default: return;
    }

    if(NULL != p_communicator->led_set_color_cb)
    {
        p_communicator->led_set_color_cb(color_component, write_val, p_communicator->p_ctx);
    }

    NRF_LOG_INFO("Write: UUID: %u, VAL: %u", char_uuid, write_val);
}

/** @brief Function for initializing services that will be used by the application.
 */
static bool services_init(ble_communicator_t* p_ctx)
{
    ble_estc_service_info estc_service_info;
    estc_service_info.fn_char_write_callback = estc_service_write_cb;
    estc_service_info.p_ctx = (void*)p_ctx;

    return estc_ble_service_init(&m_estc_service, &estc_service_info);
}

/** @brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    //ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {

    }
}


/** @brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/** @brief Function for initializing the Connection Parameters module.
 */
static bool conn_params_init(void)
{
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    if(NRF_SUCCESS != ble_conn_params_init(&cp_init))
        return false;

    return true;
}

/** @brief Function for initializing the Advertising functionality.
 */
static bool advertising_init(void)
{
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_NO_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.srdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.srdata.include_appearance = false;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    if(NRF_SUCCESS != ble_advertising_init(&m_advertising, &init))
        return false;

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

    return true;
}

/** @brief Function for starting advertising.
 */
static bool advertising_start(void)
{
    if(NRF_SUCCESS != ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST))
        return false;

    return true;
}

bool ble_communicaror_init(ble_communicator_t* p_ctx, ble_comm_init_t* p_init_params)
{
    p_ctx->led_set_color_cb = p_init_params->led_set_color_cb;
    p_ctx->p_ctx            = p_init_params->p_ctx;

    if(!ble_stack_init())
        return false;

    if(!gap_params_init())
        return false;

    if(!gatt_init())
        return false;

    if(!advertising_init())
        return false;

    if(!services_init(p_ctx))
        return false;

    if(!conn_params_init())
        return false;

    if(!advertising_start())
        return false;

    return true;
}