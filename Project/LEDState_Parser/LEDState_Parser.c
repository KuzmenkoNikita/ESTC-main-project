#include "LEDState_Parser.h"
#include "nordic_common.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define  SZ_HSV_CMD_INFO    "HSV <H> <S> <V> - Set HSV LED color H = [0:360], S = [0:100], V = [0:100]\n\r"
#define  SZ_RGB_CMD_INFO    "RGB <R> <G> <B> - Set RGB LED color R = [0:255], G = [0:255], B = [0:255]\n\r"
#define  SZ_HELP_CMD_INFO   "help - print help message\n\r"   
/* ******************************************************** */
typedef bool (*FnParseCmdParams)(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len);

/* ******************************************************** */
typedef struct 
{
    const char*          sz_cmd_name;
    FnParseCmdParams     fn_param_parser;
    const char*          sz_help_info;
}SCmdCtx;

/* ******************************************************** */
static const char* parser_next_word(const char* sz);
static bool parser_parse_help_cmd_params(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len);
static bool parser_parse_set_rgb_cmd_params(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len);
static bool parser_parse_set_hsv_cmd_params(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len);
static bool parser_check_isdigits(const char* sz_cmd);
/* ******************************************************** */

static SCmdCtx msCmdCtx[] = {{"help", parser_parse_help_cmd_params, SZ_HELP_CMD_INFO},
                            {"RGB ", parser_parse_set_rgb_cmd_params, SZ_RGB_CMD_INFO},
                            {"HSV ", parser_parse_set_hsv_cmd_params, SZ_HSV_CMD_INFO}};

/* ******************************************************** */
static bool parser_check_isdigits(const char* sz_cmd)
{
    uint32_t cmd_len = strlen(sz_cmd);
    for(uint32_t i = 0; i < cmd_len; ++i)
    {
        if(isdigit((int)sz_cmd[i]) == 0 && isspace((int)sz_cmd[i]) == 0)
        {
            return false;
        }
    }

    return true;
}
/* ******************************************************** */
static void parser_exec_help_cmd(SLEDStateParserInst* ps_inst)
{
    if(!ps_inst)
        return;

    if(ps_inst->help_request != NULL)
    {
        const uint32_t mass_size = ARRAY_SIZE(msCmdCtx);
        const char* m_sz_info[mass_size];

        for(int i = 0; i < mass_size; ++i)
        {
            m_sz_info[i] = msCmdCtx[i].sz_help_info;
        }

        ps_inst->help_request(ps_inst->p_data, m_sz_info, mass_size);
    }
}
/* ******************************************************** */
static bool parser_parse_help_cmd_params(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len)
{
    if(!ps_inst || !sz_cmd)
        return false;

    if(strlen(sz_cmd) == name_len)
    {
        parser_exec_help_cmd(ps_inst);

        return true; 
    }
    else if (0 == parser_next_word(sz_cmd))
    {
        parser_exec_help_cmd(ps_inst);

        return true;
    }

    return false;
}

/* ******************************************************** */
static bool parser_parse_set_rgb_cmd_params(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len)
{
    const uint8_t params_cnt = 3;
    uint32_t m_params[params_cnt];

    if(!ps_inst || !sz_cmd)
        return false;

    if(strlen(sz_cmd) <= name_len)
    {
        return false;
    }

    if(!parser_check_isdigits(sz_cmd + name_len))
    {
        return false;
    }

    for(uint32_t i = 0; i < params_cnt; ++i)
    {
        sz_cmd = parser_next_word(sz_cmd);

        if(0 != sz_cmd)
        {
            m_params[i] = atoi(sz_cmd);
        }
        else
        {
            return false;
        }
    }

    if(ps_inst->set_rgb != NULL)
    {
        for(uint32_t i = 0; i < params_cnt; ++i)
        {
            if(m_params[i] > 255)
                return false;
        }

        ps_inst->set_rgb(m_params[0], m_params[1], m_params[2], ps_inst->p_data);
    }

    return true;

}
/* ******************************************************** */
static bool parser_parse_set_hsv_cmd_params(SLEDStateParserInst* ps_inst, const char* sz_cmd, uint32_t name_len)
{
    const uint8_t params_cnt = 3;
    uint32_t m_params[params_cnt];

    if(!ps_inst || !sz_cmd)
        return false;

    if(strlen(sz_cmd) <= name_len)
    {
        return false;
    }

    if(!parser_check_isdigits(sz_cmd + name_len))
    {
        return false;
    }

    for(uint32_t i = 0; i < params_cnt; ++i)
    {
        sz_cmd = parser_next_word(sz_cmd);

        if(0 != sz_cmd)
        {
            m_params[i] = atoi(sz_cmd);
        }
        else
        {
            return false;
        }
    }

    if(ps_inst->set_hsv != NULL)
    {
        if(m_params[0] > 360 || m_params[1] > 100 || m_params[2] > 100)
            return false;

        ps_inst->set_hsv(m_params[0], m_params[1], m_params[2], ps_inst->p_data);
    }

    return true;   
}
/* ******************************************************** */
static const char* parser_next_word(const char* sz)
{
    uint32_t space_cnt = 0;

    if(!sz)
        return NULL;

    for(int i = 0; i < strlen(sz); ++i)
    {
        if(0 != isspace((int)sz[i]))
        {
            ++space_cnt;
        }
        else if (space_cnt)
        {
            return sz + i;
        }
    }
    
    return NULL;
}

/* ******************************************************** */
void parser_parse_cmd(SLEDStateParserInst* ps_inst, const char* sz_cmd)
{
    if(!ps_inst)
        return;

    for(int i = 0; i < ARRAY_SIZE(msCmdCtx); ++i)
    {
        uint32_t name_len = strlen(msCmdCtx[i].sz_cmd_name);
        if(0 == strncmp(sz_cmd, msCmdCtx[i].sz_cmd_name, name_len))
        {
            if(NULL != msCmdCtx[i].fn_param_parser)
            {
                if(!msCmdCtx[i].fn_param_parser(ps_inst, sz_cmd, name_len))
                {
                    ps_inst->cmd_error(ps_inst->p_data);
                }
            }

            return;
        }
    }

    ps_inst->cmd_error(ps_inst->p_data);

    return;
} 
/* ******************************************************** */
bool parser_init(SLEDStateParserInst* ps_instance, const SLEDStateParserInfo* ps_init_params)
{
    if(!ps_instance || !ps_init_params)
        return false;

    ps_instance->cmd_error      = ps_init_params->cmd_error;
    ps_instance->help_request   = ps_init_params->help_request;
    ps_instance->p_data         = ps_init_params->p_data;
    ps_instance->set_hsv        = ps_init_params->set_hsv;
    ps_instance->set_rgb        = ps_init_params->set_rgb;

    return true;
}





