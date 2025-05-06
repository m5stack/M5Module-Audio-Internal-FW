/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_ex.h"
#include "RGB.h"
#include "myflash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 寄存器地址
#define LIN_MIC_PC_EN_ADDR (0x00)
#define HP_MODE_ADDR (0x10)
#define HP_DET_ADDR (0x20)
#define RGB_LIGHT_ADDR (0x30)
#define RGB1_STATUS_ADDR (0x40)
#define RGB2_STATUS_ADDR (0x43)
#define RGB3_STATUS_ADDR (0x46)
#define FLASH_WRITE_BACK (0xF0)
#define IAP_UPDATE_ADDR (0xFD)
#define SW_VER_REG_ADDR (0xFE)
#define I2C_ADDR_REG_ADDR (0xFF)
// 寄存器
__IO static uint8_t lin_mic_pc_en_reg = 0;
__IO static uint8_t hp_mode_reg = 0;
__IO static uint8_t hp_det_reg = 0;
__IO uint8_t rgb_light_reg = 0;
__IO RGB_Color_TypeDef rgb_reg[3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
__IO static uint8_t sw_ver_reg = 0;
__IO uint8_t i2c_addr_reg = 0;

#define FIRMWARE_VERSION (1)
#define I2C1_ADDR_BASE (0x33)
#define rgb_reg_LIGHT_BASE (10)
#define APPLICATION_ADDRESS (0x08001800) // APP的起始地址
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
__IO uint32_t i2c_stop_timeout_delay = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void iap_set() {
	uint8_t i;
	uint32_t *pVecTab = (uint32_t*) (0x20000000);
	for (i = 0; i < 48; i++) {
		*(pVecTab++) = *(__IO uint32_t*) (APPLICATION_ADDRESS + (i << 2));
	}
#if 1 // STM32
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	__HAL_SYSCFG_REMAPMEMORY_SRAM();
#else // AMP32
	RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SYSCFG);
	/* Remap SRAM at 0x00000000 */
	SYSCFG->CFG1_B.MMSEL = SYSCFG_MemoryRemap_SRAM;
#endif

}

void lin_mic_pc_enable(void) {
	HAL_GPIO_WritePin(LIN_MIC_PU_EN_GPIO_Port, LIN_MIC_PU_EN_Pin, GPIO_PIN_SET);
	lin_mic_pc_en_reg = MICROPHONE_ON;
}

void lin_mic_pc_disable(void) {
	HAL_GPIO_WritePin(LIN_MIC_PU_EN_GPIO_Port, LIN_MIC_PU_EN_Pin,
			GPIO_PIN_RESET);
	lin_mic_pc_en_reg = MICROPHONE_OFF;
}

void set_national_hp_mode(void) {
	HAL_GPIO_WritePin(HP_MODE_SET_GPIO_Port, HP_MODE_SET_Pin, GPIO_PIN_RESET);
	hp_mode_reg = NATIONAL_STANDARD;
}
void set_americal_hp_mode(void) {
	HAL_GPIO_WritePin(HP_MODE_SET_GPIO_Port, HP_MODE_SET_Pin, GPIO_PIN_SET);
	hp_mode_reg = AMERICA_STANDARD;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == HP_DET_Pin) {
		hp_det_reg = HEADPHONE_INSERTION;
	}
}

void reg_init(void) {
	uint8_t flag = 0;
	// 麦克风状态初始化
	if (get_lin_mic_pc_en_status() == 0xFF) {
		lin_mic_pc_en_reg = MICROPHONE_ON;
//		set_lin_mic_pc_en_status(MICROPHONE_ON);
		flag++;
	} else {
		lin_mic_pc_en_reg = get_lin_mic_pc_en_status();
	}
	if (lin_mic_pc_en_reg == MICROPHONE_ON) {
		lin_mic_pc_enable();
	} else {
		lin_mic_pc_disable();
	}
	// 耳机模式初始化
	if (get_hp_mode_status() == 0xFF) {
		hp_mode_reg = NATIONAL_STANDARD;
//		set_hp_mode_status(NATIONAL_STANDARD);
		flag++;
	} else {
		hp_mode_reg = get_hp_mode_status();
	}
	if (hp_mode_reg == NATIONAL_STANDARD) {
		set_national_hp_mode();
	} else {
		set_americal_hp_mode();
	}
	// 检测是否有耳机插入
	if (HAL_GPIO_ReadPin(HP_DET_GPIO_Port, HP_DET_Pin) == GPIO_PIN_RESET) {
		hp_det_reg = HEADPHONE_UNPLUG;
	} else {
		hp_det_reg = HEADPHONE_INSERTION;
	}
	// rgb_reg亮度初始化
	if (get_rgb_light_status() == 0xFF) {
		rgb_light_reg = rgb_reg_LIGHT_BASE;
//		set_rgb_light_status(rgb_reg_LIGHT_BASE);
		flag++;
	} else {
		rgb_light_reg = get_rgb_light_status();
	}
	if(flag!=0){
		write_flash(lin_mic_pc_en_reg, hp_mode_reg, rgb_light_reg);
	}
	// 软件版本初始化
	sw_ver_reg = FIRMWARE_VERSION;
	// I2C地址初始化
	if (get_i2c_addr() == 0xFF) {
		i2c_addr_reg = I2C1_ADDR_BASE;
		set_i2c_addr(I2C1_ADDR_BASE);
	} else {
		i2c_addr_reg = get_i2c_addr();
	}
}

void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len) {
	uint8_t rx_buf[16];
	uint8_t tx_buf[32];
	uint8_t rx_mark[16] = { 0 };
	if (len > 1) {
		if (rx_data[0] == 0x00 && len == 2) {
			if (rx_data[1] == MICROPHONE_OFF || rx_data[1] == MICROPHONE_ON) {
				if(lin_mic_pc_en_reg != rx_data[1]){
					lin_mic_pc_en_reg = rx_data[1];
//					set_lin_mic_pc_en_status(rx_data[1]);
					if (lin_mic_pc_en_reg == MICROPHONE_ON) {
						lin_mic_pc_enable();
					} else {
						lin_mic_pc_disable();
					}
				}
			}
		} else if (rx_data[0] == 0x10 && len == 2) {
			if (rx_data[1] == NATIONAL_STANDARD
					|| rx_data[1] == AMERICA_STANDARD) {
				if(hp_mode_reg != rx_data[1]){
					hp_mode_reg = rx_data[1];
//					set_hp_mode_status(rx_data[1]);
					if (hp_mode_reg == NATIONAL_STANDARD) {
						set_national_hp_mode();
					} else {
						set_americal_hp_mode();
					}
				}

			}
		} else if (rx_data[0] == 0x30 && len == 2) {
			if (rx_data[1] >= 0 && rx_data[1] <= 100) {
				if(rgb_light_reg != rx_data[1]){
					rgb_light_reg = rx_data[1];
//					set_rgb_light_status(rx_data[1]);
					rgb_update();
				}
			}
		} else if (rx_data[0] >= 0x40 && rx_data[0] <= 0x48 && len <= 10) {
			uint8_t led_index = 0;
			uint8_t led_rgb = 0;
			for (int i = 0; i < len - 1; i++) {
				rx_buf[rx_data[0] + i - 0x40] = rx_data[1 + i];
				rx_mark[rx_data[0] + i - 0x40] = 1;
			}
			for (uint8_t i = 0; i < 9; i++) {
				if (rx_mark[i]) {
					led_index = i / 3;
					led_rgb = i % 3;
					if (led_rgb == 0) {
						rgb_reg[led_index].R = rx_buf[i];
					} else if (led_rgb == 1) {
						rgb_reg[led_index].G = rx_buf[i];
					} else if (led_rgb == 2) {
						rgb_reg[led_index].B = rx_buf[i];
					}
				}
			}
			rgb_update();
		} else if (rx_data[0] == I2C_ADDR_REG_ADDR && len == 2) {
			if (rx_data[1] >= 0x08 && rx_data[1] <= 0x77) {
				if (i2c_addr_reg != rx_data[1]) {
					i2c_addr_reg = rx_data[1];
					set_i2c_addr(rx_data[1]);
					user_i2c_init();
				}
			}
		} else if (rx_data[0] == IAP_UPDATE_ADDR && len == 2) {
			if (rx_data[1] > 0) {
				NVIC_SystemReset(); // 复位单片机触发升级
			}
		} else if (rx_data[0] == FLASH_WRITE_BACK && len == 2) {
			if (rx_data[1] == 1) {
				if (lin_mic_pc_en_reg != get_lin_mic_pc_en_status() || hp_mode_reg != get_hp_mode_status() || rgb_light_reg != get_rgb_light_status()) {
					write_flash(lin_mic_pc_en_reg, hp_mode_reg, rgb_light_reg);
				}
			}
		}
	}
	if (len == 1) {
		switch (rx_data[0]) {
		case LIN_MIC_PC_EN_ADDR:
			tx_buf[0] = lin_mic_pc_en_reg;
			i2c2_set_send_data(tx_buf, 1);
			break;
		case HP_MODE_ADDR:
			tx_buf[0] = hp_mode_reg;
			i2c2_set_send_data(tx_buf, 1);
			break;
		case HP_DET_ADDR:
			tx_buf[0] = hp_det_reg;
			i2c2_set_send_data(tx_buf, 1);
			break;
		case RGB_LIGHT_ADDR:
			tx_buf[0] = rgb_light_reg;
			i2c2_set_send_data(tx_buf, 1);
			break;
		case RGB1_STATUS_ADDR:
			i2c2_set_send_data((uint8_t*) rgb_reg, 9);
			break;
		case (RGB1_STATUS_ADDR + 1):
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 1, 8);
			break;
		case (RGB1_STATUS_ADDR + 2):
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 2, 7);
			break;
		case RGB2_STATUS_ADDR:
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 3, 6);
			break;
		case (RGB2_STATUS_ADDR + 1):
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 4, 5);
			break;
		case (RGB2_STATUS_ADDR + 2):
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 5, 4);
			break;
		case RGB3_STATUS_ADDR:
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 6, 3);
			break;
		case (RGB3_STATUS_ADDR + 1):
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 7, 2);
			break;
		case (RGB3_STATUS_ADDR + 2):
			i2c2_set_send_data(((uint8_t*) rgb_reg) + 8, 1);
			break;
		case SW_VER_REG_ADDR:
			tx_buf[0] = sw_ver_reg;
			i2c2_set_send_data(tx_buf, 1);
			break;
		case I2C_ADDR_REG_ADDR:
			tx_buf[0] = i2c_addr_reg;
			i2c2_set_send_data(tx_buf, 1);
			break;
		default:
			break;
		}
	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */
	iap_set();
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
//	MX_I2C2_Init();
	MX_IWDG_Init();
	MX_TIM17_Init();
	/* USER CODE BEGIN 2 */
	reg_init();
	user_i2c_init();
	i2c2_it_enable();
//	lin_mic_pc_enable();
	rgb_init();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		i2c_timeout_counter = 0;
		if (i2c_stop_timeout_flag) {
		   if (i2c_stop_timeout_delay < HAL_GetTick()) {
		      i2c_stop_timeout_counter++;
		      i2c_stop_timeout_delay = HAL_GetTick() + 10;
		   }
		}
		if (i2c_stop_timeout_counter > 50) {
			LL_I2C_DeInit(I2C2);
			LL_I2C_DisableAutoEndMode(I2C2);
		    LL_I2C_Disable(I2C2);
		    LL_I2C_DisableIT_ADDR(I2C2);
		    user_i2c_init();
		    i2c2_it_enable();
		    HAL_Delay(500);
		 }
		if (hp_det_reg == HEADPHONE_INSERTION) {
			if (HAL_GPIO_ReadPin(HP_DET_GPIO_Port, HP_DET_Pin)
					== GPIO_PIN_RESET) {
				hp_det_reg = HEADPHONE_UNPLUG;
			}
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		LL_IWDG_ReloadCounter(IWDG);						   // 复位看门狗
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
	RCC_OscInitStruct.PLL.PLLN = 8;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
