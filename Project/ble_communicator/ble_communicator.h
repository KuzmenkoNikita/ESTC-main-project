#ifndef BLE_COMMUNICATOR__
#define BLE_COMMUNICATOR__

#include <stdint.h>
#include <stdbool.h>

/** @brief led color components */
typedef enum
{
    BLE_LED_COMPONENT_H,
    BLE_LED_COMPONENT_S,
    BLE_LED_COMPONENT_V
}ble_led_components;

/**
 * @brief led color component set callback
 *
 * @param color_component     color component type
 * @param value               color component value
 * @param p_ctx               contex passed to this callback
 */
typedef void (*ble_led_set_color_component)(ble_led_components color_component, uint16_t value, void* p_ctx);

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
 * @param color             color component
 * @param value             color component value
 */
bool ble_communicator_notify_color(ble_communicator_t* p_ctx, ble_led_components color, uint16_t value);


#endif /* BLE_COMMUNICATOR__ */