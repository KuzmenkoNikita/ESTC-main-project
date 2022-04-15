#ifndef BLE_COMMUNICATOR__
#define BLE_COMMUNICATOR__

#include <stdint.h>


typedef struct 
{
    
}ble_communicator_t;


typedef struct 
{
    /* data */
}ble_comm_init_t;




int32_t ble_communicaror_init(ble_communicator_t* p_ctx, ble_comm_init_t* p_init_params);




#endif /* BLE_COMMUNICATOR__ */