#include "LEDState_Parser.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_CMD_NAME_LEN            30
#define CMD_COUNT                   3

/* ******************************************************** */
typedef int32_t (*FnParseCmdParams)(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len);

/* ******************************************************** */
typedef struct 
{
    char                sz_cmd_name[MAX_CMD_NAME_LEN];
    FnParseCmdParams    fnParamParser;
}SCmdCtx;

/* ******************************************************** */
static char* parser_next_word(char* sz);
static int32_t parser_parse_help_cmd_params(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len);
static int32_t parser_parse_set_rgb_cmd_params(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len);
static int32_t parser_parse_set_hsv_cmd_params(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len);
static bool parser_check_isdigits(char* sz_cmd);
/* ******************************************************** */

static SCmdCtx msCmdCtx[CMD_COUNT] =    {{"help", parser_parse_help_cmd_params},
                                        {"RGB ", parser_parse_set_rgb_cmd_params},
                                        {"HSV ", parser_parse_set_hsv_cmd_params}};

/* ******************************************************** */
static bool parser_check_isdigits(char* sz_cmd)
{
    for(uint32_t i = 0; i < strlen(sz_cmd); ++i)
    {
        if(isdigit((int)sz_cmd[i]) == 0)
        {
            if(isspace((int)sz_cmd[i]) == 0)
            {
                return false;
            }
        }
    }

    return true;
}
/* ******************************************************** */
static int32_t parser_parse_help_cmd_params(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len)
{
    if(!ps_inst || !sz_cmd)
        return -1;

    if(strlen(sz_cmd) == name_len)
    {
        if(ps_inst->help_request != NULL)
        {
            ps_inst->help_request(ps_inst->p_data);
        }

        return 0; 
    }
    else if (0 == parser_next_word(sz_cmd))
    {
        if(ps_inst->help_request != NULL)
        {
            ps_inst->help_request(ps_inst->p_data);
        }

        return 0;
    }

    return -1;
}

/* ******************************************************** */
static int32_t parser_parse_set_rgb_cmd_params(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len)
{
    const uint8_t params_cnt = 3;
    uint32_t m_params[params_cnt];

    if(!ps_inst || !sz_cmd)
        return -1;

    if(strlen(sz_cmd) <= name_len)
    {
        return -1;
    }

    if(!parser_check_isdigits(sz_cmd + name_len))
    {
        return -1;
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
            return -1;
        }
    }

    if(ps_inst->set_rgb != NULL)
    {
        for(uint32_t i = 0; i < params_cnt; ++i)
        {
            if(m_params[i] > 255)
                return -1;
        }

        ps_inst->set_rgb(m_params[0], m_params[1], m_params[2], ps_inst->p_data);
    }

    return 0;

}
/* ******************************************************** */
static int32_t parser_parse_set_hsv_cmd_params(SLEDStateParserInst* ps_inst, char* sz_cmd, uint32_t name_len)
{
    const uint8_t params_cnt = 3;
    uint32_t m_params[params_cnt];

    if(!ps_inst || !sz_cmd)
        return -1;

    if(strlen(sz_cmd) <= name_len)
    {
        return -1;
    }

    if(!parser_check_isdigits(sz_cmd + name_len))
    {
        return -1;
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
            return -1;
        }
    }

    if(ps_inst->set_hsv != NULL)
    {
        if(m_params[0] > 360 || m_params[1] > 100 || m_params[2] > 100)
            return -1;

        ps_inst->set_hsv(m_params[0], m_params[1], m_params[2], ps_inst->p_data);
    }

    return 0;   
}
/* ******************************************************** */
static char* parser_next_word(char* sz)
{
    uint32_t space_cnt = 0;

    if(!sz)
        return  0;

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
    
    return 0;
}

/* ******************************************************** */
void parser_parse_cmd(SLEDStateParserInst* ps_inst, char* sz_cmd)
{
    if(!ps_inst)
        return;

    for(int i = 0; i < CMD_COUNT; ++i)
    {
        if(0 == strncmp(sz_cmd, msCmdCtx[i].sz_cmd_name, strlen(msCmdCtx[i].sz_cmd_name)))
        {
            if(0 != msCmdCtx[i].fnParamParser)
            {
                if(0 != msCmdCtx[i].fnParamParser(ps_inst, sz_cmd, strlen(msCmdCtx[i].sz_cmd_name)))
                {
                    ps_inst->cmd_error(ps_inst->p_data);
                }

                return;
            }
        }
    }

    ps_inst->cmd_error(ps_inst->p_data);

    return;
} 
/* ******************************************************** */
uint32_t parser_init(SLEDStateParserInst* ps_instance, const SLEDStateParserInfo* ps_init_params)
{
    if(!ps_instance || !ps_init_params)
        return -1;

    ps_instance->cmd_error      = ps_init_params->cmd_error;
    ps_instance->help_request   = ps_init_params->help_request;
    ps_instance->p_data         = ps_init_params->p_data;
    ps_instance->set_hsv        = ps_init_params->set_hsv;
    ps_instance->set_rgb        = ps_init_params->set_rgb;

    return 0;
}





