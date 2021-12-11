#include "LEDState_Parser.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_CMD_NAME_LEN            30
#define CMD_COUNT                   3


typedef int32_t (*FnParseCmdParams)(char* szParams);


typedef struct 
{
    char                sz_cmd_name[MAX_CMD_NAME_LEN];
    FnParseCmdParams    fnParamParser;
}SCmdCtx;


static SCmdCtx msCmdCtx[CMD_COUNT] =    {{"help ", 0},
                                        {"RGB ", 0},
                                        {"HSV ", 0}};




/* ******************************************************** */
char* parser_next_word(char* sz)
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
int32_t parser_parse_cmd(char* szParams)
{
    for(int i = 0; i < CMD_COUNT; ++i)
    {
        if(0 == strncpy(szParams, msCmdCtx[i].sz_cmd_name, strlen(msCmdCtx[i].sz_cmd_name)))
        {
            if(0 != msCmdCtx[i].fnParamParser(szParams + strlen(msCmdCtx[i].sz_cmd_name)))
            {
                retuen -1;
            }
        }
    }

    return 0;
} 
/* ******************************************************** */







