/**
 * Copyright 2022 Evgeniy Morozov
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
*/

#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;
    ble_uuid_t service_uuid;
    // TODO: 3. Add service UUIDs to the BLE stack table using `sd_ble_uuid_vs_add`
    ble_uuid128_t base_uuid = {ESTC_UUID_BASE};
    error_code = sd_ble_uuid_vs_add(&base_uuid, &service->uuid_type);
    VERIFY_SUCCESS(error_code);

    service_uuid.type   = service->uuid_type;
    service_uuid.uuid   = ESTC_UUID_SERVICE;

    // TODO: 4. Add service to the BLE stack using `sd_ble_gatts_service_add`
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &service->service_handle);
    VERIFY_SUCCESS(error_code);

    estc_ble_add_characteristics(service);

    return NRF_SUCCESS;
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    //ret_code_t error_code = NRF_SUCCESS;

    // TODO: 6.1. Add custom characteristic UUID using `sd_ble_uuid_vs_add`, same as in step 4
    ble_uuid_t service_uuid;
    service_uuid.type = service->uuid_type;
    service_uuid.uuid = ESTC_UUID_CHAR_1;

    // TODO: 6.5. Configure Characteristic metadata (enable read and write)
    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_ext_props.reliable_wr  = 1;
    char_md.char_ext_props.wr_aux       = 1;

    char_md.char_props.auth_signed_wr   = 1;
    char_md.char_props.broadcast        = 0;
    char_md.char_props.indicate         = 0;
    char_md.char_props.notify           = 0;
    char_md.char_props.read             = 1;
    char_md.char_props.write            = 1;
    char_md.char_props.write_wo_resp    = 1;

    char_md.char_user_desc_max_size     = 10;
    char_md.char_user_desc_size         = 0;

    char_md.p_cccd_md                   = NULL;
    char_md.p_char_pf                   = NULL;
    char_md.p_char_user_desc            = NULL;
    char_md.p_sccd_md                   = NULL;
    char_md.p_user_desc_md              = NULL;
    


    // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc            = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth         = 0;
    attr_md.vlen            = 0;
    attr_md.wr_auth         = 0;

    // TODO: 6.6. Set read/write security levels to our attribute metadata using `BLE_GAP_CONN_SEC_MODE_SET_OPEN`
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // TODO: 6.2. Configure the characteristic value attribute (set the UUID and metadata)
    ble_gatts_attr_t attr_char_value = { 0 };

    attr_char_value.init_len    = sizeof(uint8_t);
    attr_char_value.init_offs   = 0;
    attr_char_value.max_len     = sizeof(uint8_t);
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.p_uuid      = &service_uuid;
    attr_char_value.p_value     = 0;

    // TODO: 6.7. Set characteristic length in number of bytes in attr_char_value structure


    // TODO: 6.4. Add new characteristic to the service using `sd_ble_gatts_characteristic_add`

        return sd_ble_gatts_characteristic_add(service->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &service->char1_handle);

    //return NRF_SUCCESS;
}
