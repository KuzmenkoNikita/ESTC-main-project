#ifndef _COLOR_DATA_STRUCTURES_
#define _COLOR_DATA_STRUCTURES_
/* *********************************************************************************************** */
#include <stdint.h>
/* *********************************************************************************************** */
#define CRC_MASK                0xFF000000
#define CRC_OFFSET              24

#define LED_HUE_MASK            0x000003FF
#define HUE_OFFSET              0

#define LED_SAT_MASK            0x0001FC00
#define SAT_OFFSET              10

#define LED_VAL_MASK            0x00FE0000
#define VAL_OFFSET              17

/* *********************************************************************************************** */
static inline uint8_t get_crc_from_flashdata(uint32_t flash_val)
{
    return (flash_val & CRC_MASK) >> CRC_OFFSET;
}
/* *********************************************************************************************** */
static inline uint8_t get_led_val_from_flashdata(uint32_t flash_val)
{
    return (flash_val & LED_VAL_MASK) >> VAL_OFFSET;
}
/* *********************************************************************************************** */
static inline uint8_t get_led_sat_from_flashdata(uint32_t flash_val)
{
    return (flash_val & LED_SAT_MASK) >> SAT_OFFSET;
}
/* *********************************************************************************************** */
static inline uint16_t get_led_hue_from_flashdata(uint32_t flash_val)
{
    return (flash_val & LED_HUE_MASK) >> HUE_OFFSET;
}
/* *********************************************************************************************** */
static inline uint32_t set_crc_to_flashdata(uint32_t flash_val, uint8_t crc)
{
    flash_val &= ~CRC_MASK;
    return flash_val | (crc << CRC_OFFSET);
}
/* *********************************************************************************************** */
static inline uint32_t set_led_val_to_flashdata(uint32_t flash_val, uint8_t led_val)
{
    flash_val &= ~LED_VAL_MASK;
    return (flash_val | ((uint32_t)led_val << VAL_OFFSET));
}
/* *********************************************************************************************** */
static inline uint32_t set_led_sat_to_flashdata(uint32_t flash_val, uint8_t led_sat)
{
    flash_val &= ~LED_SAT_MASK;
    return (flash_val | ((uint32_t)led_sat << SAT_OFFSET));
}
/* *********************************************************************************************** */
static inline uint32_t set_led_hue_to_flashdata(uint32_t flash_val, uint8_t led_hue)
{
    flash_val &= ~LED_HUE_MASK;
    return (flash_val | ((uint32_t)led_hue << HUE_OFFSET));
}


#endif /* _COLOR_DATA_STRUCTURES_ */