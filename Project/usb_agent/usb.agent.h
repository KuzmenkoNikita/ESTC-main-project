#ifndef NRF52840_USB_AGENT
#define NRF52840_USB_AGENT

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

int32_t usb_agent_init(void);

bool usb_agent_process(size_t* p_cmd_size);

void usb_agent_get_cmd_buf(char* p_dest_buf, size_t dest_buf_size);

void usb_agent_send_buf(const char* p_buf, size_t size);


#endif /* NRF52840_USB_AGENT */