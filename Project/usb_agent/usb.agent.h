#ifndef NRF52840_USB_AGENT
#define NRF52840_USB_AGENT

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief USB agent initialization
 * 
 * @return 0 if OK, -1 if ERROR
 */
int32_t usb_agent_init(void);

/**
 * @brief USB agent processing
 * 
 * @param p_cmd_size pointer to var, where cmd size will be saved.
 * 
 * @return true if cmd is ready
 */
bool usb_agent_process(size_t* p_cmd_size);

/**
 * @brief paste cmd to user array
 * 
 * @param p_dest_buf pointer to array, where cmd will be saved.
 * @param dest_buf_size - user buf size
 */
int32_t usb_agent_get_cmd_buf(char* p_dest_buf, size_t dest_buf_size);

/**
 * @brief send user array to USB
 * 
 * @param p_buf pointer to array to send
 * @param size - array size
 * 
 * @return 0 if OK, -1 if ERROR
 */
int32_t usb_agent_send_buf(const char* p_buf, size_t size);

/**
 * @brief reset cmd array
 */
void usb_agent_reset_cmd_buf(void);


#endif /* NRF52840_USB_AGENT */