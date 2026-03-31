/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "WS2812.h"
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
  MX_DMA_Init();
  MX_TIM3_Init();
  /* Ensure WS2812 initialized after TIM/DMA/GPIO are ready */
  ws2812_Init();
  /* USER CODE BEGIN 2 */
 
  /* Application state */
  float white_brightness = 0.0f; /* 0.0 .. 1.0 */
  uint8_t mode_flag = 0;         /* 0 = white, 1 = rainbow, 2 = running */

  /* Button debounce / repeat state */
  uint8_t up_held = 0, down_held = 0;
  uint32_t last_debounce_up = 0, last_debounce_down = 0;
  uint32_t last_repeat_up = 0, last_repeat_down = 0;
  uint8_t sim_pressed = 0;

  /* Double click detection for Key_down */
  uint32_t last_down_release = 0;
  uint8_t first_click_detected = 0;

  /* Double click detection for Key_up */
  uint32_t last_up_release = 0;
  uint8_t first_up_click_detected = 0;

  const uint32_t DOUBLE_CLICK_TIME = 300; /* Max interval for double click */

  /* Rainbow incremental state */
  int rainbow_phase = 0;
  uint32_t last_rainbow_tick = HAL_GetTick();
  const uint32_t RAINBOW_PERIOD_MS = 50; /* frame period for rainbow */
  uint32_t running_light_speed = 150;    /* frame period for running light (slower) */
  const uint32_t DEBOUNCE_MS = 50;
  const uint32_t REPEAT_INITIAL_MS = 300;
  const uint32_t REPEAT_INTERVAL_MS = 100;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();
    uint8_t up = HAL_GPIO_ReadPin(Key_up_GPIO_Port, Key_up_Pin);
    uint8_t down = HAL_GPIO_ReadPin(Key_down_GPIO_Port, Key_down_Pin);

    /* Simultaneous press: toggle mode on press (active low) */
    if (up == GPIO_PIN_RESET && down == GPIO_PIN_RESET)
    {
      if (!sim_pressed)
      {
        sim_pressed = 1;
        mode_flag = (mode_flag + 1) % 3;
        /* immediate refresh on mode switch */
        if (mode_flag == 0)
        {
          ws2812_set_all_white(white_brightness);
        }
        else
        {
          rainbow_phase = 0;
        }
        /* Reset double click states to prevent conflict */
        first_up_click_detected = 0;
        first_click_detected = 0;
      }
    }
    else
    {
      sim_pressed = 0;

      /* UP button handling (active low) */
      if (up == GPIO_PIN_RESET)
      {
        if (!up_held && (now - last_debounce_up) > DEBOUNCE_MS)
        {
          up_held = 1;
          last_repeat_up = now;
          
          /* Check for double click */
          if (first_up_click_detected && (now - last_up_release) <= DOUBLE_CLICK_TIME)
          {
            /* Double click detected - Max Brightness */
            white_brightness = 1.0f;
            if (mode_flag == 0) ws2812_set_all_white(white_brightness);
            first_up_click_detected = 0; /* Reset double click state */
          }
          else
          {
            /* Normal single click action */
            white_brightness += 0.05f;
            if (white_brightness > 1.0f) white_brightness = 1.0f;
            if (mode_flag == 0) ws2812_set_all_white(white_brightness);
          }
        }
        else if (up_held && (now - last_repeat_up) > REPEAT_INITIAL_MS)
        {
          /* auto-repeat while held */
          first_up_click_detected = 0; /* Cancel double click if held */
          white_brightness += 0.05f;
          if (white_brightness > 1.0f) white_brightness = 1.0f;
          if (mode_flag == 0) ws2812_set_all_white(white_brightness);
          last_repeat_up = now - (REPEAT_INITIAL_MS - REPEAT_INTERVAL_MS);
        }
      }
      else
      {
        if (up_held)
        {
          /* Button was just released */
          if (!first_up_click_detected)
          {
            first_up_click_detected = 1;
            last_up_release = now;
          }
        }
        
        /* Timeout check for double click */
        if (first_up_click_detected && (now - last_up_release) > DOUBLE_CLICK_TIME)
        {
          first_up_click_detected = 0;
        }

        up_held = 0;
        last_debounce_up = now;
      }

      /* DOWN button handling (active low) */
      if (down == GPIO_PIN_RESET)
      {
        if (!down_held && (now - last_debounce_down) > DEBOUNCE_MS)
        {
          down_held = 1;
          last_repeat_down = now;
          
          /* Check for double click */
          if (first_click_detected && (now - last_down_release) <= DOUBLE_CLICK_TIME)
          {
            /* Double click detected - turn off all LEDs */
            white_brightness = 0.0f;
            if (mode_flag == 0)
            {
              ws2812_set_all_white(white_brightness);
            }
            else
            {
              /* Turn off all LEDs in rainbow/running mode */
              for (uint8_t i = 0; i < WS2812_NUM; i++)
              {
                ws2812_set_rgb(i, 0, 0, 0);
              }
              ws2812_update();
              ws2812_wait_dma(100);
            }
            first_click_detected = 0; /* Reset double click state */
          }
          else
          {
            /* Normal single click action */
            white_brightness -= 0.05f;
            if (white_brightness < 0.0f) white_brightness = 0.0f;
            if (mode_flag == 0) ws2812_set_all_white(white_brightness);
          }
        }
        else if (down_held && (now - last_repeat_down) > REPEAT_INITIAL_MS)
        {
          /* Reset double click detection when button is held */
          first_click_detected = 0;
          
          white_brightness -= 0.05f;
          if (white_brightness < 0.0f) white_brightness = 0.0f;
          if (mode_flag == 0) ws2812_set_all_white(white_brightness);
          last_repeat_down = now - (REPEAT_INITIAL_MS - REPEAT_INTERVAL_MS);
        }
      }
      else
      {
        if (down_held)
        {
          /* Button was just released */
          if (!first_click_detected)
          {
            first_click_detected = 1;
            last_down_release = now;
          }
        }

        /* Timeout check */
        if (first_click_detected && (now - last_down_release) > DOUBLE_CLICK_TIME)
        {
          first_click_detected = 0;
        }
        
        down_held = 0;
        last_debounce_down = now;
      }
    }

    /* Rainbow incremental update (non-blocking) */
    if (mode_flag == 1)
    {
      if ((now - last_rainbow_tick) >= RAINBOW_PERIOD_MS)
      {
        float frequency = 0.1f;
        int center = 128;
        int width = 127;
        for (uint8_t led_id = 0; led_id < WS2812_NUM; led_id++)
        {
          uint32_t color = rainbow_color(frequency, rainbow_phase + led_id * 2, center, width);
          uint8_t r, g, b;
          color_to_rgb(color, &r, &g, &b);
          /* apply brightness scaling */
          r = (uint8_t)((float)r * white_brightness);
          g = (uint8_t)((float)g * white_brightness);
          b = (uint8_t)((float)b * white_brightness);
          ws2812_set_rgb(led_id, r, g, b);
        }
        ws2812_update();
        ws2812_wait_dma(100);
        rainbow_phase++;
        last_rainbow_tick = now;
      }
    }
    else if (mode_flag == 2)
    {
      if ((now - last_rainbow_tick) >= running_light_speed)
      {
        ws2812_running_rainbow_cycle(rainbow_phase, white_brightness);
        rainbow_phase++;
        last_rainbow_tick = now;
      }
    }

    /* Small sleep to avoid busy loop */
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
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
  while (1)
  {
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
