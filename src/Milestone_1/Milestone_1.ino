#include <LiquidCrystal.h>

// --- Motor Driver Pins ---
#define EN1          2    // PWM enable pin
#define M1A          3    // Motor direction A
#define M2A          4    // Motor direction B

// --- Encoder Pin ---
#define ENCODER_PIN  18   // Must be an interrupt-capable pin (Mega: 2,3,18,19,20,21)
                          // Changed from 2 to 18 to avoid conflict with EN1

// --- LED Pins ---
#define LED_GREEN    23
#define LED_RED      25

// --- LCD Pins ---
#define RS     52
#define Enable 50
#define D4     46
#define D5     44
#define D6     42
#define D7     40

// --- Encoder Config ---
#define ENCODER_PULSES_PER_ROTATION 20  // physical pulses per full rotation

// --- Globals ---
volatile long encoderCount = 0;   // volatile: written in ISR, read in main loop
unsigned long lastTime  = 0;
long          lastCount = 0;

LiquidCrystal lcd(RS, Enable, D4, D5, D6, D7);

// ---------------------------------------------------------------
// ISR — attached on RISING only, so each pulse = 1 count
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
  for (int16_t pwm = 255; pwm >= 0; pwm--) {  // int16_t: no uint8_t wrap-around
    analogWrite(EN1, pwm);
    delay(10);
  }
}

// ---------------------------------------------------------------
// LCD countdown helper
// ---------------------------------------------------------------
void lcdCountdown() {
  lcd.print("Starting in ");
  for (int i = 3; i >= 1; i--) {
    lcd.setCursor(12, 0);
    lcd.print(i);
    lcd.print("s");
    delay(1000);
  }
  lcd.clear();
}

// ---------------------------------------------------------------
// setup
// ---------------------------------------------------------------
void setup() {
  // LCD
  lcd.begin(16, 2);

  // Motor pins
  pinMode(EN1,  OUTPUT);
  pinMode(M1A,  OUTPUT);
  pinMode(M2A,  OUTPUT);

  // LED pins
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);

  // Encoder interrupt — RISING: 1 count per pulse (no double-count)
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countEncoder, RISING);

  // Serial
  Serial.begin(115200);
  Serial.println("Starting in 3s!");

  // Countdown on LCD
  lcdCountdown();

  // Set motor direction and ramp up
  digitalWrite(LED_RED,   HIGH);  // motor running indicator
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(M1A, HIGH);
  digitalWrite(M2A, LOW);
  motor_ramp_up();

  lastTime = millis();
}

// ---------------------------------------------------------------
// loop
// ---------------------------------------------------------------
void loop() {
  unsigned long currentTime = millis();

  // Print RPM every 1000 ms
  if (currentTime - lastTime >= 1000) {

    // Safely snapshot the volatile counter
    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    long  deltaCount = currentCount - lastCount;
    // RISING only → deltaCount pulses per second
    // RPM = (pulses/sec) / (pulses/rev) * 60
    float rpm = ((float)deltaCount / ENCODER_PULSES_PER_ROTATION) * 60.0f;

    // Serial output
    Serial.print("Encoder Count: ");
    Serial.print(currentCount);
    Serial.print(" | RPM: ");
    Serial.println(rpm, 1);

    // LCD output — row 0: count, row 1: RPM
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cnt:");
    lcd.print(currentCount);
    lcd.setCursor(0, 1);
    lcd.print("RPM:");
    lcd.print(rpm, 1);

    lastCount = currentCount;
    lastTime  = currentTime;
  }
}
