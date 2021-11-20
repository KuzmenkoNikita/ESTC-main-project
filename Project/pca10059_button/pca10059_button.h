#include <stdint.h>

#define PCA10059_TRM4DBLCLICK   0

/** @brief Enumerator used for button state */
typedef enum
{
    BTN_UNDEFINED = -1,
    BTN_PRESSED,
    BTN_RELEASED,
    BTN_DOUBLE_CLICKED
}eBtnState;

/**
 * @brief Button interrupt handler
 *
 * @param eState   button state
 * @param pData pointer to user data
 */
typedef void (*FnButtonHandler)(eBtnState eState, void* pData);

/** @brief Button init params */
typedef struct
{
    eBtnState       eBtnIrqState;
    FnButtonHandler fnBtnHandler;               /* Set 0 if not use IRQ */
    void*           pUserData;                  /* user param */
}SBtnIRQParams;

/** @brief Function used for button initialization 
*/
void pca10059_button_init(SBtnIRQParams* psInitParams);

/** @brief Function used for enable button interrupt
*/
void pca10059_button_enable_irq(void);

/** @brief Function used for disable button interrupt
*/
void pca10059_button_disable_irq(void);


/**
 * @brief Set color LED function
 *
 * @param peState   pointer to button state
 * @return 0 if OK, -1 if Error
 */
eBtnState pca10059_GetButtonState(void);