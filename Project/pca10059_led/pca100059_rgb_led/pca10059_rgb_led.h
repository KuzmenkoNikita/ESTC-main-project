#ifndef PCA10059_RGB_LED
#define PCA10059_RGB_LED

#include <stdint.h>

/** @brief PWM instance for LED module */
#define PCA10059_PWM_INST_NUM   0

/**
 * @brief Init RGB led module
 * @return 0 if OK, -1 if Error
 */
int32_t pca10059_RGBLed_init(void);

/**
 * @brief Set color LED function
 *
 * @param Red   Red color component [0..255]
 * @param Green   Green color component [0..255]
 * @param Blue   Blue color component [0..255]
 */
void pca10059_RGBLed_Set(uint8_t Red, uint8_t Green, uint8_t Blue);

#endif