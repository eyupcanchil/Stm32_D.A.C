/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
unsigned char disp[]={
        0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,
        0x88,0x83,0xC6,0xA1,0x86,0x8E,0xF7,0xFF,0xC1};

/*
0-9  : rakamlar
15   : F harfi
17   : boş karakter
18   : V gibi kullanılan karakter
*/

volatile unsigned char birler=0;
volatile unsigned char onlar=0;
volatile unsigned char yuzler=17;
volatile unsigned char binler=18;

volatile char i=0;
volatile unsigned char buton = 0;

#define SINE_TABLE_SIZE 64
#define TIMER_CLOCK_HZ 8000000UL
#define DAC_VREF_MV 3300UL

const uint8_t sineTable[SINE_TABLE_SIZE] = {
    128, 140, 153, 165, 177, 188, 199, 209,
    218, 226, 234, 240, 245, 250, 253, 255,
    255, 255, 253, 250, 245, 240, 234, 226,
    218, 209, 199, 188, 177, 165, 153, 140,
    128, 115, 102, 90, 78, 67, 56, 46,
    37, 29, 21, 15, 10, 5, 2, 0,
    0, 0, 2, 5, 10, 15, 21, 29,
    37, 46, 56, 67, 78, 90, 102, 115
};

volatile uint32_t phaseAcc = 0;
volatile uint32_t phaseStep = 0;

typedef enum
{
    MODE_VOLTAGE,
    MODE_FREQUENCY,
    MODE_RUN
} SystemMode;

volatile SystemMode mode = MODE_VOLTAGE;

volatile uint8_t amplitude_x10 = 33;    // 33 = 3.3V
volatile uint16_t frequency_hz = 100;   // başlangıç 100Hz
volatile uint8_t outputEnable = 0;

uint16_t inputValue = 0;
uint8_t inputCount = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void DAC_Write_8bit(uint8_t value);
void Display_Mode_Value(uint8_t symbol, uint16_t value);
void Display_Frequency_Value(uint16_t value);
void Display_Output_Value(uint8_t amp, uint8_t freq);
void Set_Sine_Frequency(uint16_t freq);
void buton_kontrol(void);
void System_Reset_To_Input_Mode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void DAC_Write_8bit(uint8_t value)
{
    uint32_t setMask;
    uint32_t resetMask;

    /*
     * DAC PB8-PB15 arasında.
     * BSRR ile sadece PB8-PB15 değiştirilir.
     * PB0-PB7 keypad pinlerine dokunulmaz.
     */

    setMask = ((uint32_t)value << 8);   // value bitleri PB8-PB15'e kaydırılır
    resetMask = 0xFF00;                 // PB8-PB15 temizlenecek pinler

    GPIOB->BSRR = (resetMask << 16) | setMask;
}
void System_Reset_To_Input_Mode(void)
{
    outputEnable = 0;

    HAL_TIM_Base_Stop_IT(&htim2);

    DAC_Write_8bit(0);

    phaseAcc = 0;
    phaseStep = 0;

    mode = MODE_VOLTAGE;
    inputValue = 0;
    inputCount = 0;

    amplitude_x10 = 0;
    frequency_hz = 0;

    Display_Mode_Value(18, 0);   // V 00
}

void Display_Mode_Value(uint8_t symbol, uint16_t value)
{
	/*
	 * Genlik için kullanılır:
	 * V 00 - V 33
	 */

    if(value > 99)
    {
        value = 99;
    }

    binler = symbol;       // V
    yuzler = 17;           // boş
    onlar  = value / 10;
    birler = value % 10;
}

void Display_Frequency_Value(uint16_t value)
{
    /*
     * Frekans seçimi:
     * F 00 - F 99
     */

    if(value > 99)
    {
        value = 99;
    }

    binler = 15;          // F
    yuzler = 17;          // boş
    onlar  = value / 10;
    birler = value % 10;
}
void Display_Output_Value(uint8_t amp, uint8_t freq)
{
    /*
     * Çalışma ekranı:
     * Sol 2 hane  → genlik
     * Sağ 2 hane → frekans
     *
     * Örnek:
     * amp = 20, freq = 34 → 2034
     */

    if(amp > 99)
    {
        amp = 99;
    }

    if(freq > 99)
    {
        freq = 99;
    }

    binler = amp / 10;
    yuzler = amp % 10;
    onlar  = freq / 10;
    birler = freq % 10;
}

void Set_Sine_Frequency(uint16_t freq)
{
    uint32_t sampleRate;

    if(freq < 1)
    {
        freq = 1;
    }

    if(freq > 99)
    {
        freq = 99;
    }

    /*
     * TIM2:
     * 8 MHz / (799 + 1) = 10000 Hz
     */
    sampleRate = 10000UL;

    phaseStep = ((uint64_t)freq * 4294967296ULL) / sampleRate;
}


void buton_kontrol(void)
{
    /*
     * RAKAM TUŞLARI
     * Çok basılırsa sadece son 2 hane tutulur.
     *
     * Örnek:
     * 1 2 3 4 basılırsa değer 34 olur.
     */

    if(buton < 10)
    {
        if(mode == MODE_RUN)
        {
            /*
             * Çıkış çalışırken rakam tuşları yeni değer girmesin.
             * Yeni seçim için PA15 reset kullanılacak.
             */
            return;
        }

        inputValue = ((inputValue % 10) * 10) + buton;
        inputCount++;

        if(mode == MODE_VOLTAGE)
        {
            Display_Mode_Value(18, inputValue);   // Vxx
        }

        else if(mode == MODE_FREQUENCY)
        {
            Display_Frequency_Value(inputValue);  // Fxx
        }
    }


    /*
     * A TUŞU: 1 artır
     */

    if(buton == 0x0A)
    {
        if(mode == MODE_VOLTAGE)
        {
            if(inputValue < 33)
            {
                inputValue++;
            }

            Display_Mode_Value(18, inputValue);
        }

        else if(mode == MODE_FREQUENCY)
        {
            if(inputValue < 99)
            {
                inputValue++;
            }

            Display_Frequency_Value(inputValue);
        }
    }


    /*
     * B TUŞU: 1 azalt
     */

    if(buton == 0x0B)
    {
        if(mode == MODE_VOLTAGE)
        {
            if(inputValue > 0)
            {
                inputValue--;
            }

            Display_Mode_Value(18, inputValue);
        }

        else if(mode == MODE_FREQUENCY)
        {
            if(inputValue > 0)
            {
                inputValue--;
            }

            Display_Frequency_Value(inputValue);
        }
    }


    /*
     * C TUŞU: seçilen değeri sıfırla
     */

    if(buton == 0x0C)
    {
        inputValue = 0;
        inputCount = 0;

        if(mode == MODE_VOLTAGE)
        {
            Display_Mode_Value(18, 0);       // V00
        }

        else if(mode == MODE_FREQUENCY)
        {
            Display_Frequency_Value(0);      // F00
        }
    }


    /*
     * D TUŞU: ONAY
     *
     * Genlik modundaysa:
     *   Genliği onaylar, frekans moduna geçer.
     *
     * Frekans modundaysa:
     *   Frekansı onaylar, sinüsü başlatır.
     */

    if(buton == 0x0D)
    {
        if(mode == MODE_VOLTAGE)
        {
            /*
             * Genlik maksimum 50 olabilir.
             * 50 üstü girildiyse kabul edilmez, V00'a döner.
             */

        	if(inputValue > 33)
        	{
        	    /*
        	     * 50 üstü genlik kabul edilmez.
        	     * Değeri sıfırlamıyoruz.
        	     * Kullanıcı A/B/C veya rakamlarla 50 altına çekene kadar
        	     * D tuşu onay vermeyecek.
        	     */
        	    Display_Mode_Value(18, inputValue);
        	    return;
        	}

            amplitude_x10 = (uint8_t)inputValue;

            inputValue = 0;
            inputCount = 0;

            mode = MODE_FREQUENCY;
            Display_Frequency_Value(0);      // F00
        }

        else if(mode == MODE_FREQUENCY)
        {
            /*
             * Frekans 2 haneli olacak.
             * 00 girilirse 1 Hz kabul edelim.
             */

            if(inputValue < 1)
            {
                inputValue = 1;
            }

            if(inputValue > 99)
            {
                inputValue = 99;
            }

            frequency_hz = inputValue;

            phaseAcc = 0;
            Set_Sine_Frequency(frequency_hz);

            outputEnable = 1;
            HAL_TIM_Base_Start_IT(&htim2);

            mode = MODE_RUN;

            /*
             * Çalışma ekranı:
             * Sol 2 hane genlik, sağ 2 hane frekans.
             */
            Display_Output_Value(amplitude_x10, frequency_hz);

            inputValue = 0;
            inputCount = 0;
        }
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  Display_Mode_Value(18, 0);      // Başlangıçta V 00 göster

  HAL_TIM_Base_Start_IT(&htim1);  // Display tarama

  // TIM2 başlangıçta çalışmayacak.
  // Genlik ve frekans girildikten sonra başlatılacak.
  // HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15))
      {
          System_Reset_To_Input_Mode();
          HAL_Delay(300);
      }

      GPIOB->BSRR = ((uint32_t)0x000F << 16) | 0x01; // 1. satır PB0
      for(volatile int d = 0; d < 300; d++);

      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) ){
          buton = 1; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) ){
          buton = 2; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) ){
          buton = 3; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) ){
          buton = 0x0A; buton_kontrol(); HAL_Delay(200);
      }

      GPIOB->BSRR = ((uint32_t)0x000F << 16) | 0x02; // 2. satır PB1
      for(volatile int d = 0; d < 300; d++);

      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) ){
          buton = 4; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) ){
          buton = 5; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) ){
          buton = 6; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) ){
          buton = 0x0B; buton_kontrol(); HAL_Delay(200);
      }

      GPIOB->BSRR = ((uint32_t)0x000F << 16) | 0x04; // 3. satır PB2
      for(volatile int d = 0; d < 300; d++);

      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) ){
          buton = 7; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) ){
          buton = 8; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) ){
          buton = 9; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) ){
          buton = 0x0C; buton_kontrol(); HAL_Delay(200);
      }

      GPIOB->BSRR = ((uint32_t)0x000F << 16) | 0x08; // 4. satır PB3
      for(volatile int d = 0; d < 300; d++);

      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) ){
          buton = 0x0F; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) ){
          buton = 0; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) ){
          buton = 0x0E; buton_kontrol(); HAL_Delay(200);
      }
      if( HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) ){
          buton = 0x0D; buton_kontrol(); HAL_Delay(200);
      }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 7999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 799;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 PA9 PA10 PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB13 PB14
                           PB15 PB3 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_3|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB4 PB5 PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15))
	{
	    System_Reset_To_Input_Mode();
	    return;
	}
    /*
     * TIM1: 7-segment display tarama
     */
    if(htim->Instance == TIM1)
    {
        i++;

        if(i == 1)
        {
            GPIOA->ODR = 0x0000;
            GPIOA->ODR |= (disp[birler] << 4);
            GPIOA->ODR &= 0xFFF0;
            GPIOA->ODR |= 0x01;
        }

        if(i == 2)
        {
            GPIOA->ODR = 0x0000;
            GPIOA->ODR |= (disp[onlar] << 4);
            GPIOA->ODR &= 0xFFF0;
            GPIOA->ODR |= 0x02;
        }

        if(i == 3)
        {
            GPIOA->ODR = 0x0000;
            GPIOA->ODR |= (disp[yuzler] << 4);
            GPIOA->ODR &= 0xFFF0;
            GPIOA->ODR |= 0x04;
        }

        if(i == 4)
        {
            GPIOA->ODR = 0x0000;
            GPIOA->ODR |= (disp[binler] << 4);
            GPIOA->ODR &= 0xFFF0;
            GPIOA->ODR |= 0x08;
            i = 0;
        }
    }

    /*
     * TIM2: DAC sinüs üretimi
     */
    if(htim->Instance == TIM2)
    {
        uint8_t dacValue;
        uint32_t maxVoltage_mV;
        uint8_t index;

        if(outputEnable)
        {
            maxVoltage_mV = amplitude_x10 * 100;

            /*
             * 64 elemanlı tablo için üst 6 bit index olarak alınır.
             */
            index = phaseAcc >> 26;

            dacValue = ((uint32_t)sineTable[index] * maxVoltage_mV) / DAC_VREF_MV;

            DAC_Write_8bit(dacValue);

            phaseAcc += phaseStep;
        }
        else
        {
            DAC_Write_8bit(0);
        }
    }
}
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
