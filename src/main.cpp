#include <Arduino.h>
#include <DHT.h>

// Пины подключения для SOKOLF405
#define DHT_PIN PA0           // Пин для датчика DHT11
#define PELTIER_PIN PA1       // Пин управления элементом Пельтье
#define COOLER_PIN PA2        // Пин управления кулером
#define CURRENT_SENSOR_PIN PA3 // Пин для измерения тока (ADC)

// Параметры температуры
#define MIN_TEMP 2.0
#define MAX_TEMP 15.0
#define HYSTERESIS 0.5

// Параметры тока
#define MIN_CURRENT 6.0
#define MAX_CURRENT 7.0

// Создание объекта DHT
DHT dht(DHT_PIN, DHT11);

// Переменные
float currentTemp = 0;
float currentCurrent = 0;
bool peltierState = false;
bool coolerState = false;
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000;

void readSensors();
void controlSystem();
void printStatus();

void setup() {
  // Инициализация последовательного порта
  Serial.begin(115200);
  
  // Настройка пинов
  pinMode(PELTIER_PIN, OUTPUT);
  pinMode(COOLER_PIN, OUTPUT);
  pinMode(CURRENT_SENSOR_PIN, INPUT_ANALOG);
  
  // Инициализация датчика DHT11
  dht.begin();
  
  // Изначально выключаем все
  digitalWrite(PELTIER_PIN, LOW);
  digitalWrite(COOLER_PIN, LOW);
  
  Serial.println("=== Система контроля температуры SOKOLF405 ===");
  Serial.println("Целевой диапазон: +2°C до +15°C");
  Serial.println("Целевой ток: 6A-7A");
  delay(1000);
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    readSensors();
    controlSystem();
    printStatus();
    lastReadTime = currentTime;
  }
  
  // Дополнительная логика может быть добавлена здесь
  delay(100);
}

void readSensors() {
  // Чтение температуры
  currentTemp = dht.readTemperature();
  
  if (isnan(currentTemp)) {
    Serial.println("Ошибка чтения датчика температуры DHT11!");
    return;
  }
  
  // Чтение тока с датчика (калибровка под конкретный датчик)
  int adcValue = analogRead(CURRENT_SENSOR_PIN);
  
  // Преобразование ADC значения в напряжение (0-3.3V, 12-bit ADC)
  float voltage = (adcValue / 4095.0) * 3.3;
  
  // Преобразование напряжения в ток (пример для ACS712 20A)
  // ACS712 20A: 100mV/A, нулевой ток при 2.5V
  currentCurrent = (voltage - 1.65) / 0.1;
  
  // Для других датчиков тока используйте соответствующую формулу
  // Например, для INA219 или другого датчика
}

void controlSystem() {
  // Управление элементом Пельтье
  if (currentTemp > (MAX_TEMP + HYSTERESIS) && 
      currentCurrent >= MIN_CURRENT && 
      currentCurrent <= MAX_CURRENT) {
    digitalWrite(PELTIER_PIN, HIGH);
    peltierState = true;
  } 
  else if (currentTemp < (MIN_TEMP - HYSTERESIS) || 
           currentCurrent < MIN_CURRENT || 
           currentCurrent > MAX_CURRENT) {
    digitalWrite(PELTIER_PIN, LOW);
    peltierState = false;
  }
  
  // Управление кулером
  // Включаем когда работает Пельтье или температура высокая
  if (peltierState || currentTemp > MAX_TEMP + 2) {
    digitalWrite(COOLER_PIN, HIGH);
    coolerState = true;
  } else {
    digitalWrite(COOLER_PIN, LOW);
    coolerState = false;
  }
}

void printStatus() {
  Serial.print("Температура: ");
  Serial.print(currentTemp, 1);
  Serial.print("°C | Ток: ");
  Serial.print(currentCurrent, 1);
  Serial.print("A | Пельтье: ");
  Serial.print(peltierState ? "ON" : "OFF");
  Serial.print(" | Кулер: ");
  Serial.println(coolerState ? "ON" : "OFF");
  
  // Предупреждения
  if (currentCurrent < MIN_CURRENT && peltierState) {
    Serial.println("ПРЕДУПРЕЖДЕНИЕ: Ток ниже минимального! Отключение Пельтье.");
  }
  if (currentCurrent > MAX_CURRENT) {
    Serial.println("ПРЕДУПРЕЖДЕНИЕ: Ток выше максимального! Отключение Пельтье.");
  }
  if (currentTemp > MAX_TEMP + 5) {
    Serial.println("ПРЕДУПРЕЖДЕНИЕ: Температура критически высокая!");
  }
  if (currentTemp < MIN_TEMP - 2) {
    Serial.println("ПРЕДУПРЕЖДЕНИЕ: Температура критически низкая!");
  }
}