#define EN1 2
#define M1A 3
#define M2A 4
#define Encoder 5
#define LED_GREEN 23
#define LED_RED   25


void setup() {
  // put your setup code here, to run once:
  pinMode(EN1, OUTPUT);
  pinMode(M1A, OUTPUT);
  pinMode(M2A, OUTPUT);
  Serial.begin(115200);
  Serial.println("Starting in 3s!");
  delay(3000);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(M1A, HIGH);
  digitalWrite(M2A, LOW);
  for (int i = 0; i < 256; i++) {
    analogWrite(EN1,i);
    delay(50);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
