#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <DHT11.h>

// --- Motor Driver Pins ---
#define EN1 2
#define M1A 3
#define M2A 4

// --- Encoder Pin ---
#define ENCODER_PIN 18

// --- LED Pins ---
#define LED_GREEN 23
#define LED_RED 25

// --- LCD Pins ---
#define RS 52
#define Enable 50
#define D4 46
#define D5 44
#define D6 42
#define D7 40

// --- Sensor Pins ---
#define WATER_SENSOR_PIN A0
#define DHT11_SENSOR 11

// --- Encoder Config ---
#define ENCODER_PULSES_PER_ROTATION 20

// --- Sensor thresholds
#define WATER_THRESHOLD 150  
#define CURRENT_THRESHOLD 150
#define HUMIDITY_THRESHOLD 50

// --- Globals ---
volatile long encoderCount = 0;
unsigned long lastTime = 0;
unsigned long startTime = 0;
long lastCount = 0;
bool RPM_CHECK = false;
bool lcdPage = false;

// Class Calls
Adafruit_INA219 ina219;
DHT11 dht(DHT11_SENSOR);
LiquidCrystal lcd(RS, Enable, D4, D5, D6, D7);

// ---------------------------------------------------------------
// ISR
// ---------------------------------------------------------------
void countEncoder() {
  encoderCount++;
}

// ---------------------------------------------------------------
// Motor helpers
// ---------------------------------------------------------------
void motor_ramp_up() {
  for (int16_t pwm = 0; pwm <= 255; pwm++) {
    analogWrite(EN1, pwm);
    delay(10);
  }
}

void motor_ramp_down() {
  for (int16_t pwm = 255; pwm >= 0; pwm--) {
    analogWrite(EN1, pwm);
    delay(10);
  }
}

// ---------------------------------------------------------------
// Safe INA219 read — returns -1 if bus not responding
// ---------------------------------------------------------------
float safeCurrent() {
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() != 0) return -1.0f;
  return ina219.getCurrent_mA();
}

float safePower() {
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() != 0) return -1.0f;
  return ina219.getPower_mW();
}

float safeVoltage() {
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() != 0) return -1.0f;
  return ina219.getBusVoltage_V();
}

// ---------------------------------------------------------------
// LCD countdown
// ---------------------------------------------------------------
void lcdCountdown() {
  lcd.clear();
  for (int i = 3; i >= 1; i--) {
    lcd.setCursor(0, 0);
    lcd.print("Starting in ");
    lcd.print(i);
    lcd.print("s ");
    delay(1000);
    lcd.clear();
  }
}

// ---------------------------------------------------------------
// Fault handler — stops motor, lights red LED, halts
// ---------------------------------------------------------------
void fault(const char* line1, const char* line2) {
  motor_ramp_down();
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  Serial.print("FAULT: ");
  Serial.print(line1);
  Serial.print(" | ");
  Serial.println(line2);
  while (1)
    ;
}

void printData(float rpm, float voltage, float current, float power, uint16_t H20, uint16_t humidity) {
  Serial.print(" | RPM: ");
  Serial.print(rpm, 1);
  Serial.print(" | V: ");
  Serial.print(voltage, 2);
  Serial.print(" | mA: ");
  Serial.print(current, 1);
  Serial.print(" | mW: ");
  Serial.print(power, 1);
  Serial.print(" | H2O: ");
  Serial.println(H20);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RPM:");
  lcd.print(rpm, 1);
  lcd.print(" H: ");
  lcd.print(humidity);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("V:");
  lcd.print(voltage, 1);
  lcd.print(" mA:");
  lcd.print(current, 0);
}

// ---------------------------------------------------------------
// setup
// ---------------------------------------------------------------
void setup() {
  lcd.begin(16, 2);

  pinMode(EN1, OUTPUT);
  pinMode(M1A, OUTPUT);
  pinMode(M2A, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(ENCODER_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countEncoder, RISING);

  Serial.begin(115200);

  // I2C scan
  Wire.begin();
  bool ina219Found = false;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      Serial.println(addr, HEX);
      if (addr == 0x40) ina219Found = true;
    }
  }

  if (!ina219Found || !ina219.begin()) {
    Serial.println("INA219 not found — power sensing disabled");
    // non-fatal: continue without it
  }

  lcdCountdown();

  uint16_t waterReading = analogRead(WATER_SENSOR_PIN);
  Serial.print(waterReading);
  if (waterReading < WATER_THRESHOLD) {
    fault("No Water Detected!", "Not Starting");  // halts inside
  }

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(M1A, HIGH);
  digitalWrite(M2A, LOW);

  startTime = millis();
  motor_ramp_up();
  lastTime = millis();
}

// ---------------------------------------------------------------
// loop
// ---------------------------------------------------------------
void loop() {
  unsigned long currentTime = millis();

  // --- Water sensor check (every loop, fast) ---
  uint16_t waterReading = analogRead(WATER_SENSOR_PIN);
  if (waterReading < WATER_THRESHOLD) {
    fault("No Water Detected!", "Shutting Down");  // halts inside
  }

  // --- 1 second reporting block ---
  if (currentTime - lastTime >= 1000) {

    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    long deltaCount = currentCount - lastCount;
    float rpm = ((float)deltaCount / ENCODER_PULSES_PER_ROTATION) * 60.0f;

    // RPM startup check
    if (!RPM_CHECK && rpm <= 105 && currentTime - startTime > 3000) {
      fault("Motor Failed to", "Reach Top Speed!");
    } else {
      digitalWrite(LED_GREEN, HIGH);
      RPM_CHECK = true;
    }

    // INA219 readings
    float voltage = safeVoltage();
    float current = safeCurrent();
    float power = safePower();

    int16_t humidity = dht.readHumidity();

    if (humidity != DHT11::ERROR_CHECKSUM && humidity != DHT11::ERROR_TIMEOUT) {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println("%");
    }
    if (humidity >= HUMIDITY_THRESHOLD) {
      fault("Leak Detected", "Shutting Down");
    }

    if (current >= CURRENT_THRESHOLD) {
      fault("Overcurrent Fault", "Shutting Down");
    }
    printData(rpm, voltage, current, power, waterReading, humidity);
    lastCount = currentCount;
    lastTime = currentTime;
  }
}
