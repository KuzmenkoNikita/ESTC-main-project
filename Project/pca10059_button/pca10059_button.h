#include <stdint.h>

/** @brief Enumerator used for button state */
typedef enum
{
    BTN_UNDEFINED = -1,
    BTN_PRESSED,
    BTN_RELEASED
}eBtnState;

/** @brief Function used for button initialization 
*/
void pca10059_button_init(void);

/**
 * @brief Set color LED function
 *
 * @param peState   pointer to button state
 * @return 0 if OK, -1 if Error
 */
int pca10059_GetButtonState(eBtnState* peState);