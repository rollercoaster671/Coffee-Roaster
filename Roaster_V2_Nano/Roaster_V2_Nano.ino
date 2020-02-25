/*
  Stepper Motor Test
  stepper-test01.ino
  Uses MA860H or similar Stepper Driver Unit
  Has speed control & reverse switch
  
  DroneBot Workshop 2019
  https://dronebotworkshop.com
*/
 
// Defin pins
 
//int reverseSwitch = 2;  // Push button for reverse
int driverPUL = 3;    // PUL- pin
int driverDIR = 4;    // DIR- pin
int driverENA = 2;
int ledpin = 5;

int spd = A0;     // Potentiometer
int onoffread = 6;
  int sensorValue = 0;
  int outputValue = 0; 
// Variables
 
int pd = 200;       // Pulse Delay period
int pdfast = 400;
int pdslow = 2000;
int accel = 3000;
double aspd = 1000;
boolean setdir = LOW; // Set Direction
boolean onoff = HIGH;
 
// Interrupt Handler
 
void revmotor (){
 
  setdir = !setdir;
  
}
 
 
void setup() {
  pinMode (onoffread, INPUT_PULLUP);
  pinMode(ledpin, OUTPUT);
  pinMode (driverPUL, OUTPUT);
  pinMode (driverDIR, OUTPUT);
  pinMode (driverENA, OUTPUT);
  digitalWrite (driverENA, LOW);
  //attachInterrupt(digitalPinToInterrupt(reverseSwitch), revmotor, FALLING);
  Serial.begin(9600); 
}
 
void loop() {
    //sensorValue = analogRead(spd);
    //pd = map(sensorValue, 0, 1023, 0,200); 
    //pd = 400;
    onoff = digitalRead(onoffread);



    if (onoff == HIGH) {
      digitalWrite(ledpin, LOW);
    if (aspd < pdslow) {
    aspd = aspd += (aspd/accel); }
    if (aspd > (pdslow-500)) {
    digitalWrite(driverENA, LOW);
    }
    digitalWrite(driverDIR,setdir);
    digitalWrite(driverPUL,HIGH);
    delayMicroseconds(aspd);
    digitalWrite(driverPUL,LOW);
    delayMicroseconds(aspd);
    
    }
    
    if (onoff == LOW) {
      digitalWrite(ledpin, HIGH);
    digitalWrite(driverENA, HIGH);
    if (aspd > pdfast) {
    aspd = aspd -= ( aspd/accel); }
    digitalWrite(driverDIR,setdir);
    digitalWrite(driverPUL,HIGH);
    delayMicroseconds(aspd);
    digitalWrite(driverPUL,LOW);
    delayMicroseconds(aspd);}
   
    

    
    //Serial.print("sensor = ");
    //Serial.print(pd);
    //Serial.print("\t output = ");
    //Serial.println(aspd);
  


}
