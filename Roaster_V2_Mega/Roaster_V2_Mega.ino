#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <Servo.h>
#include <max6675.h>
#include <ModbusRtu.h>
#include <AccelStepper.h>

Servo gasvalve;
Servo ChargeDump;

uint16_t au16data[16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 };

Modbus slave;

int valvepos = 0;
char  Alarm_String[8] = {0};
unsigned long flamewaittemp;
unsigned long flamewaitinf;
int GasSwitchState = 0;
int LastGasSwitchState = 0;
int TimeGasSwitched = 0;
int GasGracePeriod = 3000; //3 second gas on grace period
int GasSafetyPurge = 5000; //5 second gas safety purge 
int GasStartupPos = 50; // 50 startup position
int GasONOFF = 0;
int ExhaustMaxTemp = 120;
int LastAlarmSilenceSwitchState = 0;
int timesilenced = 0;

int ChaffPot = A0;
int ChaffPotValue = 0;
int ChaffPotOutput = 0;
int GasCtrlVlv = A1;
int GasCtrlVlvValue = 0;

int ChaffCollector = 3;

int ThermoDOamb =5 ;
int ThermoCSamb =6;
int ThermoCLKamb =7;

MAX6675 thermocoupleAmb(ThermoCLKamb, ThermoCSamb, ThermoDOamb);

int ThermoDObean =8;
int ThermoCSbean =9;
int ThermoCLKbean =10;

int GasValvePin = 11;

int flame_sensor = 12;

MAX6675 thermocoupleBean(ThermoCLKbean, ThermoCSbean, ThermoDObean);

int ThermoDOexh =22;
int ThermoCSexh =23;
int ThermoCLKexh =24;

MAX6675 thermocoupleExh(ThermoCLKexh, ThermoCSexh, ThermoDOexh);

int ThermoDOfl = 25;
int ThermoCSfl =26;
int ThermoCLKfl =27;

MAX6675 thermocoupleFl(ThermoCLKfl, ThermoCSfl, ThermoDOfl);

int GasSolenoid = 28;

int StirDir = 29;
int StirStep = 30;

int ChargeDumpServo = 32;

//Switches
int GasEnableSwitch = 34;
int DrumAirSwitch = 36;
int CoolTraySwitch = 38;

//LEDs
int FlameOnLED = 39;
int FlameAlarmLED = 40;
int FlameArmedLED = 41;
int DrumAirLED = 47;
int CoolingTrayLED = 35;
int AlarmLED = 42;

int AlarmBuzz = 43;

int ChargeButton = 44;

int EStop = 45;

int AlarmSilence = 46;


#define motorInterfaceType 1

AccelStepper StirStepper = AccelStepper(motorInterfaceType, StirStep, StirDir);


int flame_detected;

LiquidCrystal_I2C lcdleft = LiquidCrystal_I2C(0x27, 16,2);
LiquidCrystal_I2C lcdright = LiquidCrystal_I2C(0x23, 16,2);


void setup() {
  
  lcdleft.init();
  lcdleft.backlight();
  lcdright.init();
  lcdright.backlight();
  
  pinMode(flame_sensor, INPUT);
  pinMode(GasEnableSwitch, INPUT_PULLUP);
  pinMode(DrumAirSwitch, INPUT_PULLUP);
  pinMode(CoolTraySwitch, INPUT_PULLUP);
  pinMode(ChargeButton, INPUT_PULLUP);
  pinMode(AlarmSilence, INPUT_PULLUP);
  pinMode(EStop, INPUT_PULLUP);
  pinMode(FlameOnLED, OUTPUT);
  pinMode(FlameAlarmLED, OUTPUT);
  pinMode(FlameArmedLED, OUTPUT);
  pinMode(DrumAirLED, OUTPUT);
  pinMode(CoolingTrayLED, OUTPUT);
  pinMode(AlarmLED, OUTPUT);
  pinMode(AlarmBuzz, OUTPUT);
  pinMode(GasSolenoid, OUTPUT);
  pinMode(ChaffCollector, OUTPUT);
  StirStepper.setMaxSpeed(500);
  StirStepper.setSpeed(200);
  StirStepper.setAcceleration(50);
  gasvalve.attach(GasValvePin);
  ChargeDump.attach(ChargeDumpServo);
  slave = Modbus(1,0,0);
  slave.begin(1920);
  Serial.begin(9600);
delay(500);  
}



void loop() {

  GasCtrlVlvValue = analogRead(GasCtrlVlv);
  ChaffPotValue = analogRead(ChaffPot);
  
  flame_detected = digitalRead(flame_sensor);  

//Chaff Collector Fan Control

  if(digitalRead(DrumAirSwitch) == LOW) {
    ChaffPotOutput = map(ChaffPotValue, 0, 1023, 255, 0);
    analogWrite(ChaffCollector, ChaffPotOutput);
    digitalWrite(DrumAirLED, HIGH);
  }
  //if(thermocoupleExh.readFarenheit()>(ExhaustMaxTemp+50)) {
  //  analogWrite(ChaffCollector, 200);
  //}
  if(digitalRead(DrumAirSwitch) == HIGH) {
    analogWrite(ChaffCollector, 0);
    digitalWrite(DrumAirLED, LOW);
  }
  Serial.print("DrumAirSwitch =");
  Serial.println(digitalRead(DrumAirSwitch));
  Serial.print("ChaffPotValue =");
  Serial.println(ChaffPotValue);
  Serial.print("ChaffPotOutput =");
  Serial.println(ChaffPotOutput);
  
//______________________________________________________________________________


//Gas Control Code---------------------------------------------------------------
   GasSwitchState = digitalRead(GasEnableSwitch);
   
   if (au16data[4] == 0) {
   valvepos = map(GasCtrlVlv, 0, 1023, 0, 90);
   }
   if (flame_detected == true) {
          digitalWrite(FlameOnLED, HIGH);
   }
   if(GasSwitchState != LastGasSwitchState) {
      if(GasSwitchState == LOW){
        TimeGasSwitched = millis();
      }}

   if(GasSwitchState == LOW) {
    
      if ((millis()-TimeGasSwitched)<GasGracePeriod) {
          gasvalve.write(GasStartupPos);  
          digitalWrite(GasSolenoid, HIGH);
          digitalWrite(FlameArmedLED, HIGH);        
      }
      
      else if ((millis()-TimeGasSwitched)>GasGracePeriod && flame_detected == true) {
          digitalWrite(GasSolenoid,HIGH);
          gasvalve.write(valvepos);
      }
      
      else if ((millis()-TimeGasSwitched)>GasGracePeriod && flame_detected == false) {
          digitalWrite(GasSolenoid, LOW);
          gasvalve.write(0);
          digitalWrite(FlameAlarmLED, HIGH);
          Alarm_String[1] = "Gas Safety Shutoff";
      }
        
      }
    
    else {
    digitalWrite(GasSolenoid, LOW);
    gasvalve.write(0);
    digitalWrite(FlameOnLED, LOW);
    digitalWrite(FlameAlarmLED, LOW);
    digitalWrite(FlameArmedLED, LOW);
    Alarm_String[1] = "";
    }
    
    //if((millis()-GasGracePeriod-TimeGasSwitched)<GasSafetyPurge) {
    //        analogWrite(ChaffCollector, 200);
    //    }
    LastGasSwitchState = GasSwitchState;

   

   
//_______________________________________________________________________________

//Temperatures-------------------------------------------------------------------------
  
  if(thermocoupleExh.readFarenheit()>ExhaustMaxTemp) {
    Alarm_String[2] = "HiExhTemp";
  }
    if(thermocoupleExh.readFarenheit()>(ExhaustMaxTemp+50)) {
    Alarm_String[2] = "HiExhTempShutoff";
  }


//_______________________________________________________________________________


//LCD Code-----------------------------------------------------------------------
  lcdleft.clear();  
  lcdleft.setCursor(0, 0);
  lcdleft.print(String("Bean Temp:") + String(thermocoupleBean.readFarenheit())); 
  lcdleft.setCursor(0, 1); 
  lcdleft.print(String("Ambient Temp:") + String(thermocoupleAmb.readFarenheit()));

  lcdright.clear();
  lcdright.setCursor(0, 0); 
  lcdright.print(String("Exhaust Temp:") + String(thermocoupleExh.readFarenheit())); 
  lcdright.setCursor(0, 1); 
  
  for (int i = 0; i<6; i++) { 
    lcdright.print(String("Alarm:") + String(Alarm_String[i]));
  }

//____________________________________________________________________________


//Charge Dump-----------------------------------------------------------------
    ChargeDump.write(90);
    if (ChargeButton == LOW) {
      ChargeDump.write(0);
    }
//____________________________________________________________________________

//Buzzer----------------------------------------------------------------------
  if(LastAlarmSilenceSwitchState != digitalRead(AlarmSilence)) {
    if(digitalRead(AlarmSilence) == LOW){
      timesilenced = millis();
    }
  }
  if(((millis()-timesilenced)>10000) || timesilenced == 0) {
     for (int i = 0; i<6; i++) { 
      if(Alarm_String[i] != ""){
        noTone(AlarmBuzz);
      }
     }
  }
  else {
     tone(AlarmBuzz, 50);
  }
  //____________________________________________________________________________

//Cooling Tray Code-----------------------------------------------------------
  if(digitalRead(CoolTraySwitch) == LOW) {
    digitalWrite(CoolingTrayLED, HIGH);
    StirStepper.moveTo(5000000);
    StirStepper.run();
  } 
//____________________________________________________________________________

//Artisan Communication Code--------------------------------------------------
au16data[2] = ((uint16_t) (thermocoupleBean.readFarenheit()*100));

au16data[3] = ((uint16_t) (thermocoupleAmb.readFarenheit()*10));

slave.poll( au16data, 16 );
  
   valvepos = 2*au16data[4];
//_____________________________________________________________________________



}
