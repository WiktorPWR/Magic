/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MPU6050_interface.h"
#include "ai_platform.h"
#include "network.h"      // Zmieniono na relatywną ścieżkę (zakładając poprawny Include Path w CMake)
#include "network_data.h"
#include <string.h>       // Dla memset i memcpy
#include <stdio.h>        // Dla sprintf
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
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// Zmienne dla AI
ai_handle network = AI_HANDLE_NULL;
ai_buffer *ai_input;
ai_buffer *ai_output;
static ai_network_params params;
/* * POPRAWKA 1: Wymuszenie wyrównania buforów do 32 bajtów (aligned(32)).
 * Zapobiega to błędowi PRECISERR (BusFault) podczas dostępu silnika AI do danych.
 */

__attribute__((aligned(32))) static uint8_t pool0[11264];

__attribute__((aligned(32))) static float data_ins[300]; 
__attribute__((aligned(32))) static float data_outs[4];

/* * POPRAWKA 2: Zwiększony bufor UART, aby sprintf nie wykroczył poza pamięć.
 */
char uart_buf[128]; 
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * POPRAWKA 3: Kompletna i bezpieczna funkcja inicjalizacji.
 * Przekazuje uchwyt 'network' do funkcji pobierających bufory wag i aktywacji.
 */
void MX_X_CUBE_AI_Init(void) {
    ai_error err;

    // 1. Utworzenie instancji
    err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"CREATE ERR\r\n", 12, 100);
        return;
    }

    // 2. Czyścimy całą strukturę params (w tym unię i obie struktury wewnątrz)
    memset(&params, 0, sizeof(ai_network_params));

    // 3. Konfiguracja wag (pobieramy domyślne z modelu)
    params.params = ai_network_data_weights_buffer_get(AI_HANDLE_PTR(network));

    // 4. RĘCZNA KONFIGURACJA AKTYWACJI (zgodnie z Twoją strukturą ai_buffer)
    // Używamy AI_BUFFER_FORMAT_NONE lub 0, bo to surowy bufor pamięci dla silnika
    params.activations.format    = 0; 
    params.activations.data      = AI_HANDLE_PTR(pool0); // Twój bufor 11KB
    params.activations.meta_info = NULL;
    params.activations.flags     = 0;
    params.activations.size      = 11264; // Liczba bajtów

    // Opcjonalnie: kształt bufora (często wymagane, by nie był zerem)
    // Jeśli Twoja struktura ma n_chunks, ustaw go na 1
    //params.activations.shape.n_chunks = 1; 

    // 5. Przekazanie wszystkiego do silnika
    if (!ai_network_init(network, &params)) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"INIT FAIL\r\n", 11, 100);
        while(1);
    }
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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  MPU6050_Init();
  HAL_UART_Transmit(&huart1, (uint8_t*)"SYSTEM START\r\n", 14, 100);

  I2C_Scan(); // Skanowanie I2C w poszukiwaniu urządzeń (dla debugowania)
  // Inicjalizacja AI
  MX_X_CUBE_AI_Init();

  // Pobranie wskaźników do wejść i wyjść modelu
  ai_input = ai_network_inputs_get(network, NULL);
  ai_output = ai_network_outputs_get(network, NULL);

  // SPRAWDZENIE: Czy wskaźniki są poprawne?
  if ((ai_input == NULL) || (ai_output == NULL)) {
      HAL_UART_Transmit(&huart1, (uint8_t*)"GET BUFFERS ERR\r\n", 17, 100);
      while(1); 
  }
  // Powiązanie naszych fizycznych tablic z silnikiem AI
  ai_input->data = AI_HANDLE_PTR(data_ins);
  ai_output->data = AI_HANDLE_PTR(data_outs);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 1. Odczyt danych z sensora MPU6050. 
       Funkcja ta powinna wypełnić tablicę ML_Input (50 próbek x 6 osi). */
    if (MPU6050_Read_And_Set_ML_Input(&mpu6050_data) == HAL_OK) 
    {
        /* 2. Kopiowanie danych do bufora wejściowego sieci.
           Używamy (float*) dla ML_Input, aby poprawnie skopiować 300 wartości float. */
        memcpy(data_ins, ML_Input, 300 * sizeof(float));

        /* 3. Uruchomienie obliczeń sieci neuronowej.
           Jeśli tu następuje HardFault, upewnij się, że stos (Stack) w CubeMX 
           jest zwiększony do 0x2000. */
        if (ai_network_run(network, &ai_input[0], &ai_output[0]) != 1) 
        {
            HAL_UART_Transmit(&huart1, (uint8_t*)"AI Run Error!\r\n", 15, 100);
        }
        else 
        {
            // 4. Wyniki klasyfikacji znajdują się w data_outs.
            // Mnożymy przez 100, aby uzyskać wartość procentową (np. 0.85 -> 85)
            int p0 = (int)(data_outs[0] * 100);
            int p1 = (int)(data_outs[1] * 100);
            int p2 = (int)(data_outs[2] * 100);
            int p3 = (int)(data_outs[3] * 100);

            // Używamy formatu %d zamiast %f
            int len = sprintf(uart_buf, "G0: %d%% | G1: %d%% | G2: %d%% | G3: %d%%\r\n", p0, p1, p2, p3);

            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, len, 100);

            /* 5. Prosta logika sterowania - wybieramy gest z pewnością powyżej 80%. */
            if (data_outs[0] > 0.8f) {
                HAL_UART_Transmit(&huart1, (uint8_t*)"WYKRYTO: GEST 0\r\n", 17, 100);
            } 
            else if (data_outs[1] > 0.8f) {
                HAL_UART_Transmit(&huart1, (uint8_t*)"WYKRYTO: GEST 1\r\n", 17, 100);
            }
            else if (data_outs[2] > 0.8f) {
                HAL_UART_Transmit(&huart1, (uint8_t*)"WYKRYTO: GEST 2\r\n", 17, 100);
            }
            else if (data_outs[3] > 0.8f) {
                HAL_UART_Transmit(&huart1, (uint8_t*)"WYKRYTO: GEST 3\r\n", 17, 100);
            }
        }
    }
    else 
    {
        /* Błąd odczytu z I2C / MPU6050 */
        HAL_UART_Transmit(&huart1, (uint8_t*)"MPU6050 Error!\r\n", 16, 100);
    }

    /* Krótka przerwa, aby nie zapchać procesora i UARTA */
    HAL_Delay(50);
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : mode_button_1_Pin mode_button_2_Pin mode_button_3_Pin start_recording_Pin */
  GPIO_InitStruct.Pin = mode_button_1_Pin|mode_button_2_Pin|mode_button_3_Pin|start_recording_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
