#include <LiquidCrystal.h>

#define RS     12
#define RW     11
#define Enable 2
#define D0     3
#define D1     4
#define D2     5
#define D3     6
#define D4     7
#define D5     8
#define D6     9
#define D7     10

LiquidCrystal lcd(RS, RW, Enable, D0, D1, D2, D3, D4, D5, D6, D7);

void setup() {
    Serial.begin(115200);
    while(!Serial){};
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Voltage:");
    Serial.println("Start of Program");
}

void loop() {
    int raw = analogRead(A0);
    float voltage = (raw / 1023.0) * 5.0;
    long Time = millis()/1000;
    lcd.setCursor(0, 1);
    lcd.print(voltage, 3);  // 2 decimal places
    lcd.print("V   ");      // trailing spaces clear old characters
    int voltageInt = static_cast<int>(voltage*1000); // Convert to integer for serial output
    delay(50);
    Serial.println(voltageInt);

}