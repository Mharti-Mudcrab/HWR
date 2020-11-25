float outputValue1;
float outputValue2;

void setup() 
{
  Serial.begin(9600);
  pinMode(A2, INPUT);
  digitalWrite(A2, LOW);
  pinMode(A4, INPUT);
  digitalWrite(A4, LOW);
}

void loop() 
{
  outputValue1 = analogRead(A2);
  outputValue2 = analogRead(A4);
  Serial.print(outputValue1);
  Serial.print("\t");
  Serial.println(outputValue2);

  delay(20);
}
