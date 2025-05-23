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
#include "crc.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stm32g0xx_hal_flash_ex.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    NOEVENT                  = 0,  // not enevt happen
    EVENT_OPCOD_NOTYET_READ  = 1,  // operation code not been read
    EVENT_OPCOD_READ         = 2,  // operation code has been readed
    EVENT_OPCOD_SEND         = 3,  // Feedback the status of MCU
    EVENT_OPCOD_BUSY_RECEIVE = 4   // I2C is in Busy of receive status
    // EVENT_OPCOD_BUSY_SEND =5 //i2C is busy of send
} EventStatus;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
typedef void (*pFunction)(void);
/*this address is define */
#define APPLICATION_ADDRESS (0x08001800)
#define FW_LENGTH           (((uint32_t)0x7800) - ((uint32_t)0x1800))
#define FW_CRC_ADDR         (((uint32_t)0x08007800) - 4)
/* Error codes used to make the red led blinking */
#define ERROR_ERASE              (0x01)
#define ERROR_PROG               (0x02)
#define ERROR_HALF_PROG          (0x04)
#define ERROR_PROG_FLAG          (0x08)
#define ERROR_WRITE_PROTECTION   (0x10)
#define ERROR_FETCH_DURING_ERASE (0x20)
#define ERROR_FETCH_DURING_PROG  (0x40)
#define ERROR_SIZE               (0x80)
#define ERROR_ALIGNMENT          (0x100)
#define ERROR_NOT_ZERO           (0x200)
#define ERROR_UNKNOWN            (0x400)
#define ERROR_I2C                (0x01)
#define ERROR_HSI_TIMEOUT        (0x55)
#define ERROR_PLL_TIMEOUT        (0xAA)
#define ERROR_CLKSWITCH_TIMEOUT  (0xBB)

#define OPC_WREN  (uint8_t)(0x06)
#define OPC_USRCD (uint8_t)(0x77)

#define STM32G0xx_PAGE_SIZE              (0x800)
#define STM32G0xx_FLASH_PAGE0_STARTADDR  (0x8000000)
#define STM32G0xx_FLASH_PAGE1_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE2_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 2 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE3_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 3 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE4_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 4 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE5_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 5 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE6_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 6 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE7_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 7 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE8_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 8 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE9_STARTADDR  (STM32G0xx_FLASH_PAGE0_STARTADDR + 9 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE10_STARTADDR (STM32G0xx_FLASH_PAGE0_STARTADDR + 10 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE11_STARTADDR (STM32G0xx_FLASH_PAGE0_STARTADDR + 11 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE12_STARTADDR (STM32G0xx_FLASH_PAGE0_STARTADDR + 12 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE13_STARTADDR (STM32G0xx_FLASH_PAGE0_STARTADDR + 13 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE14_STARTADDR (STM32G0xx_FLASH_PAGE0_STARTADDR + 14 * STM32G0xx_PAGE_SIZE)
#define STM32G0xx_FLASH_PAGE15_STARTADDR (STM32G0xx_FLASH_PAGE0_STARTADDR + 15 * STM32G0xx_PAGE_SIZE)

// #define FLASH_PAGE_SIZE         ((uint32_t)0x00000800)   /* FLASH Page Size */
#define FLASH_USER_START_ADDR STM32G0xx_FLASH_PAGE5_STARTADDR /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR   STM32G0xx_FLASH_PAGE6_STARTADDR /* End @ of user Flash area */
#define DATA_32               ((uint32_t)0x22446688)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint16_t I2C_Receive_Counter = 0;
volatile uint8_t Receive_Buffer[2056];
volatile EventStatus i2c_event = NOEVENT;
volatile uint16_t error;
volatile uint32_t NbrOfPage    = 0x00;
volatile uint32_t EraseCounter = 0x00, Address = 0x00;
volatile uint32_t JumpAddress;
volatile pFunction JumpToApplication;
volatile uint8_t opcode;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t compute_fw_crc32(void)
{
    uint32_t crcsum, crc_read, len;
    uint8_t *pdata = (uint8_t *)APPLICATION_ADDRESS;
    len            = FW_LENGTH - 4;
    crc_read       = *(uint32_t *)FW_CRC_ADDR;
    crcsum         = HAL_CRC_Calculate(&hcrc, (uint32_t *)pdata, len) ^ 0xffffffff;
    if (crc_read == crcsum)
        return 1;
    else
        return 0;
}

/**
 * @brief  Gets the page of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The page of a given address
 */
static uint32_t get_page(uint32_t Addr)
{
    return (Addr - STM32G0xx_FLASH_PAGE0_STARTADDR) / FLASH_PAGE_SIZE;
}

/**
 * Brief   This function handles I2C2 interrupt request.
 * Param   None
 * Retval  I2C2 always as slave of i2c conmunication
 */
void I2C2_IRQHandler(void)
{
    uint32_t I2C_InterruptStatus = I2C2->ISR; /* Get interrupt status */

    if ((I2C_InterruptStatus & I2C_ISR_ADDR) == I2C_ISR_ADDR) /* Check address match */
    {
        I2C2->ICR |= I2C_ICR_ADDRCF; /* Clear address match flag*/

        if ((I2C2->ISR & I2C_ISR_DIR) == I2C_ISR_DIR) /* Check if transfer direction is read (slave transmitter) */
        {
            I2C2->CR1 |= I2C_CR1_TXIE;    /* Set transmit IT /status*/
            i2c_event = EVENT_OPCOD_SEND; /* Set I2C  entor transmit mode*/

        } else /*Write operation, slave receive status*/
        {
            I2C2->CR1 |= I2C_CR1_RXIE;            /* Set receive IT /status*/
            i2c_event = EVENT_OPCOD_BUSY_RECEIVE; /* Set I2C  entor receive mode*/
        }

        I2C2->CR1 |= I2C_CR1_STOPIE;  // Enable STOP interrupt

    } else if ((I2C_InterruptStatus & I2C_ISR_TXIS) == I2C_ISR_TXIS) {
        // add some application code in this place
    }
    /*check RXDR is not empty*/
    else if ((I2C_InterruptStatus & I2C_ISR_RXNE) == I2C_ISR_RXNE) {
        // I2C_ISR_RXNE add you code in this place Tomas Li add
        Receive_Buffer[I2C_Receive_Counter++] = I2C2->RXDR;
        i2c_event                             = EVENT_OPCOD_BUSY_RECEIVE;  // slave is busy for receive data

    } else if ((I2C_InterruptStatus & I2C_ISR_NACKF) == I2C_ISR_NACKF) {
    }
    /*check Stop event happen */
    if ((I2C_InterruptStatus & I2C_ISR_STOPF) == I2C_ISR_STOPF) {
        I2C2->ICR |= I2C_ICR_STOPCF;  // clear the STOP interrupt Flag

#if 1
        switch (i2c_event) {
            case EVENT_OPCOD_BUSY_RECEIVE:  // slave receive status Stop flag
                I2C_Receive_Counter = 0;
                i2c_event           = EVENT_OPCOD_NOTYET_READ;

                I2C2->CR1 &= ~(I2C_CR1_STOPIE);  // Disable all interrupt.except Error interrupt
                I2C2->CR2 |= I2C_CR2_NACK;       // set feedback Nack in next event
                I2C2->CR1 |= I2C_CR1_ADDRIE;
                break;
            case EVENT_OPCOD_SEND:                                              // slave send stop
                I2C2->ICR |= I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF;  // clear the STOP interrupt Flag
                I2C2->CR1 |= I2C_CR1_ADDRIE;
                i2c_event = NOEVENT;

                break;
            default:

                break;
        }
#endif

    } else {
        error = ERROR_I2C; /* Report an error */
    }
}

void reset_all_periph(void)
{
    LL_I2C_DeInit(I2C2);
    LL_I2C_DisableAutoEndMode(I2C2);
    LL_I2C_Disable(I2C2);
    LL_I2C_DisableIT_ADDR(I2C2);
    SysTick->CTRL = 0;
    SYSCFG->CFGR1 &= SYSCFG_CFGR1_MEM_MODE;
    NVIC->ICER[0] = 0xFFFFFFFF;
    NVIC->ICPR[0] = 0xFFFFFFFF;
    /* Set EXTICRx registers to reset value */
    EXTI->EXTICR[0] = 0;
    EXTI->EXTICR[1] = 0;
    EXTI->EXTICR[2] = 0;
    EXTI->EXTICR[3] = 0;
    /* Set CFGR2 register to reset value: clear SRAM parity error flag */
    SYSCFG->CFGR2 |= (uint32_t)SYSCFG_CFGR2_SRAM_PE;
    LL_RCC_DeInit();
    HAL_CRC_DeInit(&hcrc);
    HAL_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
}

void jump_app(void)
{
    if (compute_fw_crc32()) {
        /*check the application address context whether avilible*/
        if (((*(__IO uint32_t *)APPLICATION_ADDRESS) & 0x2FFE0000) == 0x20000000) {
            reset_all_periph();
            SCB->VTOR = APPLICATION_ADDRESS;
            /* Jump to user application */
            JumpAddress       = *(__IO uint32_t *)(APPLICATION_ADDRESS + 4);
            JumpToApplication = (pFunction)JumpAddress;
            /* Initialize user application's Stack Pointer */
            __set_MSP(*(__IO uint32_t *)APPLICATION_ADDRESS);
            JumpToApplication();
        }
    }
}

void write_code(void)
{
    uint16_t Number_Bytes_Transferred = 0;
    uint32_t Add_Flash, end_add_flash;
    uint64_t Data       = 0;
    uint16_t Data_index = 8;
    uint32_t PageError  = 0;  
    FLASH_EraseInitTypeDef My_Flash;
    uint32_t PageNum = 0;

    Add_Flash     = Receive_Buffer[1] << 24 | Receive_Buffer[2] << 16 | Receive_Buffer[3] << 8 | Receive_Buffer[4] << 0;
    end_add_flash = Add_Flash + 2048;

    Number_Bytes_Transferred = (Receive_Buffer[5] << 8) + Receive_Buffer[6];

    if (Number_Bytes_Transferred > 0) {
        if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_CFGBSY) != 0x00U) {
            *(uint32_t *)(Add_Flash + 600) = 12323;
            FLASH->SR                      = FLASH_SR_CLEAR;
        }
    step_unlock:
        if (HAL_FLASH_Unlock() != HAL_OK) {
            goto step_unlock;
        }

        PageNum = get_page(Add_Flash);

        My_Flash.TypeErase = FLASH_TYPEERASE_PAGES;  
        My_Flash.Page      = PageNum;
        My_Flash.NbPages   = 1;  

    step_erase:
        FLASH_WaitForLastOperation(50);
        if (HAL_FLASHEx_Erase(&My_Flash, &PageError) != HAL_OK) {
            goto step_erase;
        }  

        while (Add_Flash < end_add_flash) {
            Data = Receive_Buffer[Data_index] | ((uint64_t)Receive_Buffer[Data_index + 1] << 8) |
                   ((uint64_t)Receive_Buffer[Data_index + 2] << 16) | ((uint64_t)Receive_Buffer[Data_index + 3] << 24) |
                   ((uint64_t)Receive_Buffer[Data_index + 4] << 32) | ((uint64_t)Receive_Buffer[Data_index + 5] << 40) |
                   ((uint64_t)Receive_Buffer[Data_index + 6] << 48) | ((uint64_t)Receive_Buffer[Data_index + 7] << 56);

        step_write:
            FLASH_WaitForLastOperation(50);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Add_Flash, Data) != HAL_OK) {
                goto step_write;
            }
            Add_Flash  = Add_Flash + 8;
            Data_index = Data_index + 8;
        }
        HAL_FLASH_Lock();
        FLASH_WaitForLastOperation(50);
    }
    for (int i = 0; i < sizeof(Receive_Buffer); i++) {
        Receive_Buffer[i] = 0;
    }
}

void iap_i2c(void)
{
    /*this is a endless loop for process the data from Host side*/
    while (1) {
        // Tomas_Li_Test();//Just for Test

        if (i2c_event == EVENT_OPCOD_NOTYET_READ) {
            NVIC_DisableIRQ(I2C2_IRQn);
            i2c_event = NOEVENT;  // changed the status
            /* Read opcode */
            switch (Receive_Buffer[0]) {
                case OPC_WREN:
                    write_code();
                    break;
                case OPC_USRCD:
                    jump_app();
                    break;
                default:
                    break;
            }
            NVIC_EnableIRQ(I2C2_IRQn);
            I2C2->CR1 |= I2C_CR1_ADDRIE;  // Open address and Stop interrupt
            LL_I2C_Enable(I2C2);
            LL_I2C_EnableIT_ADDR(I2C2);
        }
    }
}

static void iap_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);

    /*Configure GPIO pin : PB6 */
    GPIO_InitStruct.Pin   = GPIO_PIN_11;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : PB7 */
    GPIO_InitStruct.Pin   = GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

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
    MX_CRC_Init();
    //  MX_I2C2_Init();
    /* USER CODE BEGIN 2 */
    iap_gpio_init();
    LL_mDelay(300);
    if ((!(!!(GPIOA->IDR & GPIO_PIN_11))) && (!(!!(GPIOA->IDR & GPIO_PIN_12)))) {
        MX_I2C2_Init();
        LL_I2C_Enable(I2C2);
        LL_I2C_EnableIT_ADDR(I2C2);
        iap_i2c();
    } else {
        jump_app();
    }
    MX_I2C2_Init();
    LL_I2C_Enable(I2C2);
    LL_I2C_EnableIT_ADDR(I2C2);
    iap_i2c();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
    }

    /* HSI configuration and activation */
    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1) {
    }

    /* Main PLL configuration and activation */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
    LL_RCC_PLL_Enable();
    LL_RCC_PLL_EnableDomain_SYS();
    while (LL_RCC_PLL_IsReady() != 1) {
    }

    /* Set AHB prescaler*/
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

    /* Sysclk activation on the main PLL */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }

    /* Set APB1 prescaler*/
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(64000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
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
