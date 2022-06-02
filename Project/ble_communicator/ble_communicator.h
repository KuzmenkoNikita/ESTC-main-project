#ifndef BLE_COMMUNICATOR__
#define BLE_COMMUNICATOR__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief led color component set callback
 *
 * @param R             Red color component 
 * @param G             Green color component  
 * @param B             Blue color component 
 * @param p_ctx               contex passed to this callback
 */
typedef void (*ble_led_set_color_component)(uint8_t R, uint8_t G, uint8_t B, void* p_ctx);

/**
 * @brief client connected callback
 * @param p_ctx contex passed to this callback
 */
typedef void (*ble_client_connected)(void* p_ctx);

/**
 * @brief client disconnected callback
 * @param p_ctx contex passed to this callback
 */
typedef void (*ble_client_disconnected)(void* p_ctx);

/** @brief BLE communicator module instance */
typedef struct 
{
    ble_led_set_color_component led_set_color_cb;
    void*                       p_ctx;
}ble_communicator_t;

/** @brief BLE communicator init params */
typedef struct 
{
    ble_led_set_color_component led_set_color_cb;
    ble_client_connected        client_connected_cb;
    ble_client_disconnected     client_disconnected_cb;
    void*                       p_ctx;
}ble_comm_init_t;

/**
 * @brief BLE communicator module initialization
 *
 * @param p_ctx             pointer to module instance
 * @param p_init_params     pointer to module init params
 */
bool ble_communicaror_init(ble_communicator_t* p_ctx, ble_comm_init_t* p_init_params);

/**
 * @brief Notify led color component (unacknowledged message)
 *
 * @param p_ctx             pointer to module instance
 * @param R             Red color component 
 * @param G             Green color component  
 * @param B             Blue color component 
 */
bool ble_communicator_send_color(ble_communicator_t* p_ctx, uint8_t R, uint8_t G, uint8_t B);


#endif /* BLE_COMMUNICATOR__ */