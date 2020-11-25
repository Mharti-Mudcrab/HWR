void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A5, INPUT);
  pinMode(A4, OUTPUT);
}

void loop() {
  int sensorValue = analogRead(A5); //read the A0 pin value
  float voltage = sensorValue * (5 / 1024.00) *2;
  Serial.print("Sensor value = ");
  Serial.print(sensorValue);
  Serial.print("\tvoltage = ");
  Serial.println(voltage);
  analogWrite(A4, analogRead(A5)/4);
  delay(1000);
}
