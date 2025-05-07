#include "RGB.h"
#include "tim.h"

// Some static predefined colors
static const RGB_Color_TypeDef RED   = {255, 0, 0};
static const RGB_Color_TypeDef GREEN = {0, 255, 0};
static const RGB_Color_TypeDef BLUE  = {0, 0, 255};
static const RGB_Color_TypeDef BLACK = {0, 0, 0};
static const RGB_Color_TypeDef WHITE = {255, 255, 255};

// 2D array storing final PWM data. Each row contains 24 bits for one LED,
// and the last row with 24 zeros is the RESET code.
static uint8_t rgb_buf[RGB_NUM + 1][24];

void rgb_init(void)
{
    activate_tim_dma();
    rgb_close_all();
}

// Initialize DMA for TIM17
void activate_tim_dma(void)
{
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, (RGB_NUM + 1) * 24);           // Set DMA transfer length
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)rgb_buf);         // Set memory address (buffer)
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&(TIM17->CCR1));  // Set peripheral address
    LL_DMA_ClearFlag_GI5(DMA1);                                                 // Clear interrupt flag
    LL_DMA_ClearFlag_TC5(DMA1);                                                 // Clear interrupt flag
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);                                 // Enable transfer complete interrupt
    LL_TIM_EnableDMAReq_CC1(TIM17);
    LL_TIM_EnableAllOutputs(TIM17);                             // Enable TIM outputs
    LL_TIM_CC_SetDMAReqTrigger(TIM17, LL_TIM_CCDMAREQUEST_CC);  // Set DMA request trigger source
    LL_TIM_CC_EnableChannel(TIM17, LL_TIM_CHANNEL_CH1);         // Enable TIM17 Channel 1
}

/**
 * @brief  Set color for a single RGB LED by converting 24-bit RGB to PWM codes
 * @param  led_id LED index
 * @param  color  RGB color structure
 */
void set_rgb_color(uint8_t led_id, RGB_Color_TypeDef color)
{
    uint8_t i;
    color.R = color.R * (rgb_light_reg / 100.0);  // Apply brightness
    color.G = color.G * (rgb_light_reg / 100.0);
    color.B = color.B * (rgb_light_reg / 100.0);
    for (i = 0; i < 8; i++) {
        rgb_buf[led_id][i] = ((color.G & (1 << (7 - i))) ? CODE_1 : CODE_0);  // Bits 0–7 store G
    }
    for (i = 8; i < 16; i++) {
        rgb_buf[led_id][i] = ((color.R & (1 << (15 - i))) ? CODE_1 : CODE_0);  // Bits 8–15 store R
    }
    for (i = 16; i < 24; i++) {
        rgb_buf[led_id][i] = ((color.B & (1 << (23 - i))) ? CODE_1 : CODE_0);  // Bits 16–23 store B
    }
}

/**
 * @brief  Update RGB LED states from the register
 */
void rgb_update(void)
{
    set_rgb_color(RGB_LED_1, rgb_reg[0]);
    set_rgb_color(RGB_LED_2, rgb_reg[1]);
    set_rgb_color(RGB_LED_3, rgb_reg[2]);
    reset_load();
    rgb_send_array();
}

/**
 * @brief  Append a final row with 24 zero PWM pulses as reset code
 *         Total duration: 24 * 1.2us = 28.8us > 24us (required)
 */
void reset_load(void)
{
    uint8_t i;
    for (i = 0; i < 24; i++) {
        rgb_buf[RGB_NUM][i] = 0;
    }
}

/**
 * @brief  Start DMA transfer to send PWM data
 */
void rgb_send_array(void)
{
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, (RGB_NUM + 1) * 24);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    LL_TIM_EnableCounter(TIM17);
}

/**
 * @brief  Display red color on a specific RGB LED
 * @param  rgb_led LED index
 */
void rgb_red(rgb_led_t rgb_led)
{
    set_rgb_color(rgb_led, RED);
    reset_load();
    rgb_send_array();
}

/**
 * @brief  Display green color on a specific RGB LED
 * @param  rgb_led LED index
 */
void rgb_green(rgb_led_t rgb_led)
{
    set_rgb_color(rgb_led, GREEN);
    reset_load();
    rgb_send_array();
}

/**
 * @brief  Display blue color on a specific RGB LED
 * @param  rgb_led LED index
 */
void rgb_blue(rgb_led_t rgb_led)
{
    set_rgb_color(rgb_led, BLUE);
    reset_load();
    rgb_send_array();
}

/**
 * @brief  Display white color on a specific RGB LED
 * @param  rgb_led LED index
 */
void rgb_white(rgb_led_t rgb_led)
{
    set_rgb_color(rgb_led, WHITE);
    reset_load();
    rgb_send_array();
}

/**
 * @brief  Turn off a specific RGB LED
 * @param  rgb_led LED index
 */
void rgb_close(rgb_led_t rgb_led)
{
    set_rgb_color(rgb_led, BLACK);
    reset_load();
    rgb_send_array();
}

/**
 * @brief  Turn off all RGB LEDs
 */
void rgb_close_all(void)
{
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        set_rgb_color(i, BLACK);
    }
    reset_load();
    rgb_send_array();
    HAL_Delay(0);
}
