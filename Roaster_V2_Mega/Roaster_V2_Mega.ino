#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <Servo.h>
#include <max6675.h>
#include <ModbusRtu.h>
#include <AccelStepper.h>

Servo gasvalve;
Servo ChargeDump;

uint16_t au16data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 };

Modbus slave(1,0,0);

int valvepos = 0;
char  Alarm_String[8] = {0};
unsigned long flamewaittemp;
unsigned long flamewaitinf;
boolean GasSwitchState = false;
int LastGasSwitchState = 0;
int TimeGasSwitched = 0;
int GasGracePeriod = 3000; //3 second gas on grace period
int GasSafetyPurge = 5000; //5 second gas safety purge 
int GasStartupPos = 50; // 50 startup position
int GasONOFF = 0;
int ExhaustMaxTemp = 120;
int LastAlarmSilenceSwitchState = 0;
int timesilenced = 0;
int x;

int ChaffPot = A2;
int ChaffPotValue = 0;
int ChaffPotOutput = 0;
int ChaffPotPercentage = 0;
int GasCtrlVlv = A3;
int GasCtrlVlvValue = 0;

int ChaffCollector = 3;

int ThermoDOamb =50 ;
int ThermoCSamb =9; 
int ThermoCLKamb =52;
unsigned int AmbTemp = 0;
unsigned long AmbTempTot = 0;
MAX6675 thermocoupleAmb(ThermoCLKamb, ThermoCSamb, ThermoDOamb);

int ThermoDObean =50;
int ThermoCSbean =33;
int ThermoCLKbean =52;
unsigned int BeanTemp = 0;
unsigned long BeanTempTot = 0;
MAX6675 thermocoupleBean(ThermoCLKbean, ThermoCSbean, ThermoDObean);


int GasValvePin = 11;

int flame_sensor = 16;



int ThermoDOexh =24;
int ThermoCSexh =23;
int ThermoCLKexh =22;
unsigned int ExhTemp = 0;
unsigned long ExhTempTot = 0;

MAX6675 thermocoupleExh(ThermoCLKexh, ThermoCSexh, ThermoDOexh);

int ThermoDOfl = 25;
int ThermoCSfl =26;
int ThermoCLKfl =27;

MAX6675 thermocoupleFl(ThermoCLKfl, ThermoCSfl, ThermoDOfl);

int GasSolenoid = 29;

int StirDir = 29;
int StirStep = 30;

//int ChargeDumpServo = 32;

//Switches
int GasEnableSwitch = 34;
int DrumAirSwitch = 36;
int ArtisanControl = 38;

//LEDs
int FlameOnLED = 39;
int FlameAlarmLED = 40;
int FlameArmedLED = 41;
int DrumAirLED = 47;
//int CoolingTrayLED = 35;
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
  pinMode(ArtisanControl, INPUT_PULLUP);
  pinMode(ChargeButton, INPUT_PULLUP);
  pinMode(AlarmSilence, INPUT_PULLUP);
  pinMode(EStop, INPUT_PULLUP);
  pinMode(FlameOnLED, OUTPUT);
  pinMode(FlameAlarmLED, OUTPUT);
  pinMode(FlameArmedLED, OUTPUT);
  pinMode(DrumAirLED, OUTPUT);
//  pinMode(CoolingTrayLED, OUTPUT);
  pinMode(AlarmLED, OUTPUT);
  pinMode(AlarmBuzz, OUTPUT);
  pinMode(GasSolenoid, OUTPUT);
  pinMode(ChaffCollector, OUTPUT);

  StirStepper.setMaxSpeed(500);
  StirStepper.setSpeed(200);
  StirStepper.setAcceleration(50);
  gasvalve.attach(GasValvePin);
//  ChargeDump.attach(ChargeDumpServo);
  
  slave.begin(19200);
  //Serial.begin(9600);
delay(500);  
}



void loop() {

//Artisan Communication Code--------------------------------------------------

au16data[2] = ((uint16_t) (BeanTemp*100));

au16data[3] = ((uint16_t) (AmbTemp*10));
//au16data[4] = ((uint16_t) (valvepos*10));

slave.poll( au16data, 16 );
if (digitalRead(ArtisanControl) == LOW ){  
   valvepos = 100-(100/90)*au16data[4];
}
//_____________________________________________________________________________

  GasCtrlVlvValue = analogRead(GasCtrlVlv);
  ChaffPotValue = analogRead(ChaffPot);
  


//Chaff Collector Fan Control

  if(digitalRead(DrumAirSwitch) == LOW) {
    if(digitalRead(ArtisanControl) == HIGH) {
    ChaffPotOutput = map(ChaffPotValue, 0, 1023, 255, 0);
    }
    if(digitalRead(ArtisanControl) == LOW) {
    ChaffPotOutput = map(au16data[5], 0, 100, 0, 255);
    }
    analogWrite(ChaffCollector, ChaffPotOutput);
    digitalWrite(DrumAirLED, HIGH);
  }

  if(digitalRead(DrumAirSwitch) == HIGH) {
    analogWrite(ChaffCollector, 0);
    ChaffPotOutput = 0;
    digitalWrite(DrumAirLED, LOW);
  }
    ChaffPotPercentage = ChaffPotOutput/2.55;

  
//______________________________________________________________________________


//Gas Control Code---------------------------------------------------------------
   GasSwitchState = digitalRead(GasEnableSwitch);
     if (GasSwitchState == LOW) {
        digitalWrite(GasSolenoid,HIGH);
     }
     if(GasSwitchState == HIGH) {
        digitalWrite(GasSolenoid,LOW);
        digitalWrite(FlameOnLED,LOW);
        digitalWrite(FlameAlarmLED,LOW);
        noTone(AlarmBuzz);
        TimeGasSwitched = 0;
     }
     

   if(digitalRead(ArtisanControl) == HIGH) {
    valvepos = map(GasCtrlVlvValue, 0, 1023, 0, 90);
   }

    flame_detected = digitalRead(flame_sensor);  
   if (flame_detected == false) {
          digitalWrite(FlameOnLED, HIGH);
   }
   if (flame_detected == true) {
      digitalWrite(FlameOnLED, LOW);
    }
   

          gasvalve.write(valvepos);  

   
//_______________________________________________________________________________

//Temperatures-------------------------------------------------------------------------


  AmbTempTot = 0;
  for(x = 0; x<8; x++) {
    AmbTempTot += thermocoupleAmb.readFarenheit();
 }
  AmbTemp = AmbTempTot/8;

  BeanTempTot = 0;
  for(x = 0; x<8; x++) {
    BeanTempTot += thermocoupleBean.readFarenheit();
  }
  BeanTemp = BeanTempTot/8;
  
  


//_______________________________________________________________________________

//Second Artisan Communication Code--------------------------------------------------

au16data[2] = ((uint16_t) (BeanTemp*100));

au16data[3] = ((uint16_t) (AmbTemp*10));
//au16data[4] = ((uint16_t) (valvepos*10));

slave.poll( au16data, 16 );
if (digitalRead(ArtisanControl) == LOW ){  
   valvepos = 100-(100/90)*au16data[4];
} 

//_____________________________________________________________________________



//LCD Code----------------------------------------------------------------------- 
  lcdleft.setCursor(0, 0);
  lcdleft.print(String("Bean Temp:") + String(BeanTemp) + String("     ")); 
//  lcdleft.print(String("Bean Temp:") + String(thermocoupleBean.readFarenheit()) + String("     "));
  
  lcdleft.setCursor(0, 1); 
  lcdleft.print(String("Ambient Temp:") + String(AmbTemp) + String("     "));
//   lcdleft.print(String("Ambient Temp:") + String(thermocoupleAmb.readFarenheit()) + String("     "));

  lcdright.setCursor(0, 0); 
  lcdright.print(String("Exhaust Temp:") + String(thermocoupleExh.readFarenheit())); 
  lcdright.setCursor(0, 1); 
  
  lcdright.print(String("Gas:") + String(100-valvepos) + String(" Air:") + String(ChaffPotPercentage) + String("     "));

//____________________________________________________________________________


//Charge Dump-----------------------------------------------------------------
    
    if (digitalRead(ChargeButton) == LOW) {
      ChargeDump.write(0);
    }
    else {
      ChargeDump.write(90);
    }
//____________________________________________________________________________





}
