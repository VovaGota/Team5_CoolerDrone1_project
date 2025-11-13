#include <Arduino.h>
#include <DHT.h>

// Пины подключения
#define DHT_PIN PA0
#define PELTIER_PIN PA1
#define COOLER_PIN PA2
#define CURRENT_SENSOR_PIN PA3

// Параметры
#define MIN_TEMP 2.0
#define MAX_TEMP 15.0
#define MIN_CURRENT 6.0
#define MAX_CURRENT 7.0
#define HYSTERESIS 0.5

DHT dht(DHT_PIN, DHT11);

float currentTemp = 0;
float currentCurrent = 0;
bool peltierState = false;
bool coolerState = false;

void setup() {
  Serial.begin(115200);
  pinMode(PELTIER_PIN, OUTPUT);
  pinMode(COOLER_PIN, OUTPUT);
  pinMode(CURRENT_SENSOR_PIN, INPUT);
  
  dht.begin();
  
  digitalWrite(PELTIER_PIN, LOW);
  digitalWrite(COOLER_PIN, LOW);
  
  Serial.println("=== Система контроля температуры ===");
}

void loop() {
  readSensors();
  controlSystem();
  printStatus();
  delay(2000);
}

void readSensors() {
  // Чтение температуры
  currentTemp = dht.readTemperature();
  
  // Чтение тока (калибровка под ваш датчик)
  int adcValue = analogRead(CURRENT_SENSOR_PIN);
  float voltage = (adcValue / 4095.0) * 3.3; // 12-bit ADC
  currentCurrent = (voltage - 1.65) / 0.1; // Пример для ACS712
}

void controlSystem() {
  // Управление Пельтье
  if (currentTemp > MAX_TEMP && 
      currentCurrent >= MIN_CURRENT && 
      currentCurrent <= MAX_CURRENT) {
    digitalWrite(PELTIER_PIN, HIGH);
    peltierState = true;
  } else if (currentTemp < MIN_TEMP || 
             currentCurrent < MIN_CURRENT || 
             currentCurrent > MAX_CURRENT) {
    digitalWrite(PELTIER_PIN, LOW);
    peltierState = false;
  }
  
  // Управление кулером
  coolerState = (peltierState || currentTemp > MAX_TEMP + 2);
  digitalWrite(COOLER_PIN, coolerState ? HIGH : LOW);
}

void printStatus() {
  Serial.print("Температура: ");
  Serial.print(currentTemp);
  Serial.print("°C | Ток: ");
  Serial.print(currentCurrent);
  Serial.print("A | Пельтье: ");
  Serial.print(peltierState ? "ON" : "OFF");
  Serial.print(" | Кулер: ");
  Serial.println(coolerState ? "ON" : "OFF");
}