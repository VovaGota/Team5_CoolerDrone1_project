#include "main.h"
#include "dht11.h"  // Предполагается, что у вас есть библиотека DHT11 для STM32

// Пины подключения для SOKOLF405
#define DHT_PIN GPIO_PIN_0
#define DHT_PORT GPIOA
#define PELTIER_PIN GPIO_PIN_1
#define PELTIER_PORT GPIOA
#define COOLER_PIN GPIO_PIN_2
#define COOLER_PORT GPIOA
#define CURRENT_SENSOR_PIN GPIO_PIN_3

// Параметры температуры
#define MIN_TEMP 2.0f
#define MAX_TEMP 15.0f
#define HYSTERESIS 0.5f

// Параметры тока
#define MIN_CURRENT 6.0f
#define MAX_CURRENT 7.0f

// Переменные
float currentTemp = 0;
float currentCurrent = 0;
uint8_t peltierState = 0;
uint8_t coolerState = 0;
uint32_t lastReadTime = 0;
const uint32_t READ_INTERVAL = 2000;

// Прототипы функций
void readSensors(void);
void controlSystem(void);
void printStatus(void);
float readCurrentSensor(void);

// Внешние зависимости (должны быть определены в вашем проекте)
extern UART_HandleTypeDef huart1;  // Для Serial
extern ADC_HandleTypeDef hadc1;    // Для ADC

void temperatureControl_Init(void) {
  // Инициализация пинов
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  // Пин Пельтье
  GPIO_InitStruct.Pin = PELTIER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PELTIER_PORT, &GPIO_InitStruct);
  
  // Пин кулера
  GPIO_InitStruct.Pin = COOLER_PIN;
  HAL_GPIO_Init(COOLER_PORT, &GPIO_InitStruct);
  
  // Изначально выключаем все
  HAL_GPIO_WritePin(PELTIER_PORT, PELTIER_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(COOLER_PORT, COOLER_PIN, GPIO_PIN_RESET);
  
  // Инициализация DHT11
  DHT11_Init();
  
  char initMsg[] = "=== Система контроля температуры SOKOLF405 ===\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t*)initMsg, strlen(initMsg), HAL_MAX_DELAY);
}

void temperatureControl_Process(void) {
  uint32_t currentTime = HAL_GetTick();
  
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    readSensors();
    controlSystem();
    printStatus();
    lastReadTime = currentTime;
  }
}

void readSensors(void) {
  // Чтение температуры с DHT11
  if (DHT11_ReadTempHumidity(&currentTemp, NULL) != DHT11_OK) {
    char errorMsg[] = "Ошибка чтения DHT11!\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)errorMsg, strlen(errorMsg), HAL_MAX_DELAY);
    return;
  }
  
  // Чтение тока
  currentCurrent = readCurrentSensor();
}

float readCurrentSensor(void) {
  float current = 0;
  uint32_t adcValue = 0;
  
  // Запуск преобразования ADC
  HAL_ADC_Start(&hadc1);
  
  // Ожидание завершения преобразования
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
    adcValue = HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);
  
  // Преобразование ADC значения в напряжение (0-3.3V, 12-bit ADC)
  float voltage = (adcValue / 4095.0f) * 3.3f;
  
  // Преобразование напряжения в ток (пример для ACS712 20A)
  // ACS712 20A: 100mV/A, нулевой ток при 2.5V
  current = (voltage - 1.65f) / 0.1f;
  
  return current;
}

void controlSystem(void) {
  // Управление элементом Пельтье
  if (currentTemp > (MAX_TEMP + HYSTERESIS) && 
      currentCurrent >= MIN_CURRENT && 
      currentCurrent <= MAX_CURRENT) {
    HAL_GPIO_WritePin(PELTIER_PORT, PELTIER_PIN, GPIO_PIN_SET);
    peltierState = 1;
  } 
  else if (currentTemp < (MIN_TEMP - HYSTERESIS) || 
           currentCurrent < MIN_CURRENT || 
           currentCurrent > MAX_CURRENT) {
    HAL_GPIO_WritePin(PELTIER_PORT, PELTIER_PIN, GPIO_PIN_RESET);
    peltierState = 0;
  }
  
  // Управление кулером
  if (peltierState || currentTemp > MAX_TEMP + 2) {
    HAL_GPIO_WritePin(COOLER_PORT, COOLER_PIN, GPIO_PIN_SET);
    coolerState = 1;
  } else {
    HAL_GPIO_WritePin(COOLER_PORT, COOLER_PIN, GPIO_PIN_RESET);
    coolerState = 0;
  }
}

void printStatus(void) {
  char statusMsg[100];
  
  snprintf(statusMsg, sizeof(statusMsg), 
           "Температура: %.1f°C | Ток: %.1fA | Пельтье: %s | Кулер: %s\r\n",
           currentTemp, currentCurrent,
           peltierState ? "ON" : "OFF",
           coolerState ? "ON" : "OFF");
  
  HAL_UART_Transmit(&huart1, (uint8_t*)statusMsg, strlen(statusMsg), HAL_MAX_DELAY);
  
  // Предупреждения
  if (currentCurrent < MIN_CURRENT && peltierState) {
    char warning[] = "ПРЕДУПРЕЖДЕНИЕ: Ток ниже минимального! Отключение Пельтье.\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)warning, strlen(warning), HAL_MAX_DELAY);
  }
  if (currentCurrent > MAX_CURRENT) {
    char warning[] = "ПРЕДУПРЕЖДЕНИЕ: Ток выше максимального! Отключение Пельтье.\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)warning, strlen(warning), HAL_MAX_DELAY);
  }
}