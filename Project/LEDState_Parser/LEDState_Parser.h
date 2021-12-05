#ifndef LED_STATE_PARSER
#define LED_STATE_PARSER

#include <stdint.h>

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
    uint16_t    R;
    uint16_t    G;
    uint16_t    B;
}SLEDStParserRGB;

/** @brief LED params params*/
typedef union 
{
    SLEDStParserHSV sHSV;
    SLEDStParserRGB sRGB;
}ULEDStateParams;

/** @brief LED params type*/
typedef enum
{
    ELEDPARSER_HSV,
    ELEDPARSER_RGB,
}ETypeParams;

/** @brief Help request CMD callback */
typedef void (*FnHelpRequestCallback)(void* pData);

/** @brief CMD error callback */
typedef void (*FnCmdErrorCallback)(void* pData);

/**
 * @brief Set led state callback
 *
 * @param puParams   pointer to params value
 * @param eTypeParam type of params
 */
typedef void (*FnSetLEDStateCallback)(ULEDStateParams* puParams, ETypeParams eTypeParam);

/** @brief Parser instance */
typedef struct 
{
    FnHelpRequestCallback   fnHelpRequest;
    FnCmdErrorCallback      fnCMDError;
    FnSetLEDStateCallback   fnSetState;
    void*                   pData;
    uint8_t                 mCMDData[20];
    uint32_t                unCMDBytesCount;
}SLEDStateParserInst;

/** @brief Parser init params */
typedef struct 
{
    FnHelpRequestCallback   fnHelpRequest;
    FnCmdErrorCallback      fnCMDError;
    FnSetLEDStateCallback   fnSetState;
    void*                   pData;
}SLEDStateParserInfo;

/**
 * @brief Set led state callback
 *
 * @param psInstance   pointer to parser instance
 * @param psInit       pointer to init params
 * 
 * @return 0 if OK, -1 if ERROR
 */
uint32_t LEDStateParser_init(SLEDStateParserInst* psInstance, SLEDStateParserInfo* psInit);

/**
 * @brief Put one byte to the parser
 *
 * @param psInstance   pointer to parser instance
 * @param Byte         Data byte 
 * 
 * @return 0 if OK, -1 if ERROR
 */
uint32_t LEDStateParser_PutByte(SLEDStateParserInst* psInstance, char Byte);

void LEDStateParser_Process(SLEDStateParserInst* psInstance);


#endif /* LED_STATE_PARSER */