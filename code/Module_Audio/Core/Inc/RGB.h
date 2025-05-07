/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __RGB_H
#define __RGB_H

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#define CODE_1  (52)  // Timer count for logic '1' signal
#define CODE_0  (25)  // Timer count for logic '0' signal
#define RGB_NUM (3)   // Number of RGB LEDs

typedef struct {
    uint8_t R;  // Red component
    uint8_t G;  // Green component
    uint8_t B;  // Blue component
} RGB_Color_TypeDef;

extern __IO uint8_t rgb_light_reg;         // RGB light status register
extern __IO RGB_Color_TypeDef rgb_reg[3];  // RGB color buffer for each LED

void rgb_init(void);                                          // Initialize RGB LED system
void activate_tim_dma(void);                                  // Initialize DMA with TIM1
void set_rgb_color(uint8_t led_id, RGB_Color_TypeDef color);  // Load color data to a specific LED
void rgb_update(void);                                        // Update RGB LED status
void reset_load(void);                                        // Append reset code (24 zeros)
void rgb_send_array(void);                                    // Send final color data array
void rgb_red(rgb_led_t rgb_led);                              // Set RGB LED to red
void rgb_green(rgb_led_t rgb_led);                            // Set RGB LED to green
void rgb_blue(rgb_led_t rgb_led);                             // Set RGB LED to blue
void rgb_white(rgb_led_t rgb_led);                            // Set RGB LED to white
void rgb_close(rgb_led_t rgb_led);                            // Turn off specified RGB LED
void rgb_close_all(void);                                     // Turn off all RGB LEDs

#ifdef __cplusplus
}
#endif

#endif /* __RGB_H */
