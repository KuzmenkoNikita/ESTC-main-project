#include "usb.agent.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_cdc_acm.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#define READ_SIZE 1
#define MAX_RECVCMD_MASS_SIZE  128
#define USB_SEND_TRY_CNT       100

static struct 
{
    char        mRxBuffer[MAX_RECVCMD_MASS_SIZE];
    uint32_t    BytesRecved;
    bool        fCMDReady;    
}sAgentCtx;

static char m_rx_buffer[READ_SIZE];

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4

APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                            usb_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

/* *************************************************************************************************** */
static ret_code_t usb_agent_save_byte(char chNewByte)
{
    ret_code_t ret;
    if(sAgentCtx.BytesRecved < MAX_RECVCMD_MASS_SIZE)
    {
        sAgentCtx.mRxBuffer[sAgentCtx.BytesRecved] = chNewByte;
        ++sAgentCtx.BytesRecved;
        ret = NRF_SUCCESS;
    }
    else
    {
        ret = NRF_ERROR_NO_MEM;
    }

    return ret;
}
/* *************************************************************************************************** */
static void usb_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
    switch (event)
    {
        
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            ret_code_t ret;
            ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
            NRF_LOG_INFO("PORT OPEN\n");
            UNUSED_VARIABLE(ret);
            break;
        } 

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            do
            {
                if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
                {
                    ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 2);

                    if(sAgentCtx.BytesRecved != 0)
                        sAgentCtx.fCMDReady = true;
                }
                else
                {
                    ret = app_usbd_cdc_acm_write(&usb_cdc_acm,
                                                m_rx_buffer,
                                                READ_SIZE);

                    usb_agent_save_byte(m_rx_buffer[0]);
                }

                ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
            } while (ret == NRF_SUCCESS);
            break;
        }
        default:
            break;
    }
}
/* *************************************************************************************************** */
int32_t usb_agent_init(void)
{
    sAgentCtx.BytesRecved = 0;
    sAgentCtx.fCMDReady = false;

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
    if(NRF_SUCCESS != app_usbd_class_append(class_cdc_acm))
        return -1;

    return 0;

}
/* *************************************************************************************************** */
bool usb_agent_process(size_t* p_cmd_size)
{
    bool state = false;

    app_usbd_event_queue_process();

    if(!p_cmd_size)
        return false;

    if(sAgentCtx.fCMDReady)
    {
        *p_cmd_size = sAgentCtx.BytesRecved;
        state = true;
    }

    return state;
}
/* *************************************************************************************************** */
void usb_agent_get_cmd_buf(char* p_dest_buf, size_t dest_buf_size)
{
    if(!p_dest_buf || !dest_buf_size)
        return;

    if(dest_buf_size < sAgentCtx.BytesRecved)
        return;

    memcpy(p_dest_buf, sAgentCtx.mRxBuffer, sAgentCtx.BytesRecved);

    sAgentCtx.BytesRecved = 0;
    sAgentCtx.fCMDReady = false;
}
/* *************************************************************************************************** */
int32_t usb_agent_send_buf(const char* p_buf, size_t size)
{
    ret_code_t ret;
    uint32_t  try_cnt = 0;

    if(!p_buf || !size)
        return -1;

    do
    {
        ret = app_usbd_cdc_acm_write(&usb_cdc_acm, p_buf, size);
        ++try_cnt;
    } while (ret != NRF_SUCCESS && try_cnt <  USB_SEND_TRY_CNT);

    return ret == NRF_SUCCESS ? 0 : -1;
}
/* *************************************************************************************************** */