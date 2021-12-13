#ifndef LED_STATE_PARSER
#define LED_STATE_PARSER

#include <stdint.h>
#include <stdbool.h>

/** @brief HSV LED params*/
typedef struct 
{
    uint16_t    H;
    uint16_t    S;
    uint16_t    V;
}SLEDStParserHSV;

/** @brief RGB LED params*/
typedef struct 
{
    uint8_t    R;
    uint8_t    G;
    uint8_t    B;
}SLEDStParserRGB;


/** @brief Help request CMD callback 
 * @param p_m_sz_info - pointer to string array with help info
 * @param array_size - array size
 * 
*/
typedef void (*fn_help_request)(void* p_data, const char** p_m_sz_info, uint32_t  array_size);

/** @brief CMD error callback */
typedef void (*fn_cmd_error)(void* p_data);

/**
 * @brief Set RGB LED state callback
 *
 * @param r         R value
 * @param g         G value
 * @param b         B value
 * 
 * @param p_data    pointer to user data
 */
typedef void (*fn_set_led_rgb)(uint8_t r, uint8_t g, uint8_t b, void* p_data);

/**
 * @brief Set HSV LED state callback
 *
 * @param h         H value
 * @param s         S value
 * @param v         V value
 * 
 * @param p_data    pointer to user data
 */
typedef void (*fn_set_led_hsv)(uint16_t h, uint8_t s, uint8_t v, void* p_data);

/** @brief Parser instance */
typedef struct 
{
    fn_help_request         help_request;
    fn_cmd_error            cmd_error;
    fn_set_led_rgb          set_rgb;
    fn_set_led_hsv          set_hsv;
    void*                   p_data;
}SLEDStateParserInst;

/** @brief Parser init params */
typedef struct 
{
    fn_help_request         help_request;
    fn_cmd_error            cmd_error;
    fn_set_led_rgb          set_rgb;
    fn_set_led_hsv          set_hsv;
    void*                   p_data;
}SLEDStateParserInfo;

/**
 * @brief Set led state callback
 *
 * @param ps_instance   pointer to parser instance
 * @param ps_init_params       pointer to init params
 * 
 * @return true if OK, false if ERROR
 */
bool parser_init(SLEDStateParserInst* ps_instance, const SLEDStateParserInfo* ps_init_params);

/**
 * @brief Set led state callback
 *
 * @param ps_instance       pointer to parser instance
 * @param ps_init_params    pointer to cmd string
 */
void parser_parse_cmd(SLEDStateParserInst* ps_instance, const char* sz_cmd);



#endif /* LED_STATE_PARSER */