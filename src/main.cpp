#include <Arduino.h>
#include <M5StickC.h>
#include <VL53L0X.h>                  //Time of flight distance sensor
#include <Wire.h>                     //i2C comms needed for VL sensor

/*
########################################################################################################
      This has been tested 4th April 2020 on M5Stick-C (Orange Casing) 
      It is a standard M5 Stick C Unit (Orange Casing) 
      Board Type = "M5Stick-C"
      TOF on white grove connector - SDA connected to G32, SCL connected to G33

      20 second count down to time the washing of hands for COVID19 outbreak

      NOTE: If unit runs down flat you may find that the screen stops working, if so then download the M5Burner from here:
            https://m5stack.com/pages/download
            Extract from the zip and run M5Burner.exe
            select UIFLOW-v1.4.5.1 on the left (it may download the firmware file),
            then at the top select Series:StickC, select the correct Com port
            Now hit Erase followed by Burn
            once the firmware has been installed and the unit restarts, the screen should start up with the UIFlow logo
            Now reload your sketch and it should work fine
            
########################################################################################################
    v0.1 Initial version
    v0.2 Added Ready Message
    v0.3 Added now rinse , also triggered by button for testing if you dont have a TOF sensor
    v0.4 Fixed bug that screen would not come on after full battery drain or 6 second button turn off - use M5.begin() instead of M5.Lcd.begin
    this is tested and deployed
*/


const char* BuildTime = __TIME__;
const char* BuildDate = __DATE__;
const char* Version = "V0.4";
#define SKETCH "Wash Hands"
#define HOSTNAME "M5StickC-1"
#define STARTUPMSGDELAY 1500           // ms to wait to display startup messages
#define SHORTVERSION true             // output just the version no on screen at startup
#define STARTUPSCREENBRIGHTNESS 200    // startup brightness
boolean M5stack = true;               //Enable M5 output text
int StdScreenBrightness = 100;
int BackGroundColor = BLUE;
String BackGround = "RED";
char Build[30];

// Used for M5 Button
const int  M5ButtonPin = 37;
uint8_t lastM5ButtonState = 1;
uint8_t M5ButtonState = 0;

// Used for Side Button
const int  SideButtonPin = 39;
uint8_t lastSideButtonState = 1;
uint8_t SideButtonState = 0;

/** START - Time Of Flight VL53L0X parameters **/
VL53L0X VL53L0Xsensor;
#define HIGH_ACCURACY
boolean TOFSensor = true;
int SensorDist = 50;
int SensorDist1 = 50;
int SensorDist2 = 50;
int SensorDist3 = 50;
int VL53L0XAddress = 0x29;
boolean Trigger = false;
String SensorStatus = "#TOF#";
int TriggerDist = 160;
/** END - Time Of Flight VL53L0X parameters **/

int SecsCountDownFrom = 20;
int MainLoopDelay = 200;

void CheckButtons()
{
    M5ButtonState = digitalRead(M5ButtonPin);
    if(!M5ButtonState && lastM5ButtonState){
        Trigger = true;
    }
    lastM5ButtonState = M5ButtonState;

    SideButtonState = digitalRead(SideButtonPin);
    if(!SideButtonState && lastSideButtonState){
        Trigger = true;
    }
    lastSideButtonState = SideButtonState;


}

void SetupVL53L0X()
{
  // Check VL53L0X TOF and set for accurate scanning
  Wire.begin();
  Wire.beginTransmission (VL53L0XAddress);
  if (Wire.endTransmission() == 0)
  {
    Serial.print (F("VL53L0X device found at 0x"));
    Serial.println (VL53L0XAddress, HEX);
    
    // only set parameters if sensor was detected
    VL53L0Xsensor.init();
    VL53L0Xsensor.setTimeout(500);
    //VL53L0Xsensor.setMeasurementTimingBudget(200000);
  }
  else
  {
    Serial.println("######  VL53L0X device NOT found   ######");
    Serial.println("######     TEST MODE - BUTTON      ######");
    TOFSensor = false;
  }
}

void CheckTOF()
{
  if (TOFSensor == true and Trigger == false)       // only read if sensor was found at startup and no current trigger
  {
    SensorDist1 = VL53L0Xsensor.readRangeSingleMillimeters();
    delay(50);
    SensorDist2 = VL53L0Xsensor.readRangeSingleMillimeters();
    delay(50);
    SensorDist3 = VL53L0Xsensor.readRangeSingleMillimeters();
    SensorDist = (SensorDist1 + SensorDist2 + SensorDist3)/3;     // take an average of 3 settings to remove 'aborations'
    
    if (VL53L0Xsensor.timeoutOccurred())
    {
      Serial.println("VL53L0X TIMEOUT");
      SensorDist = 50;            // default sensor distance if it timeouts
    }
    else
    {
    Serial.println("Distance = " + String(SensorDist));
      if ( SensorDist <= TriggerDist )         // Door triggered
      {
        Trigger = true;
      }
      else
      {
        Trigger = false;
      }
    Serial.println("Trigger = " + String(Trigger));

    }
  }
  else
  {
    CheckButtons();
  }
}


void M5OutputBuildInfo()
{
  //M5.Lcd.setBrightness(StdScreenBrightness);
  M5.Lcd.fillScreen(BackGroundColor);
  if (SHORTVERSION == true )
  {
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE, BackGroundColor);
    M5.Lcd.println(Version);
    delay(STARTUPMSGDELAY);
  }
  else
  {
    sprintf(Build, "%s %s", BuildTime, BuildDate );
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE, BackGroundColor);
    M5.Lcd.println(Version);
    M5.Lcd.println(SKETCH);
    M5.Lcd.println(BuildTime);
    M5.Lcd.println(BuildDate);
    M5.Lcd.println(HOSTNAME);
    delay(STARTUPMSGDELAY);
  }
}

void OutputBuildInfo()
{
  sprintf(Build, "%s %s", BuildTime, BuildDate );
  Serial.print(SKETCH);
  Serial.print(" ");
  Serial.println(Version);
  Serial.print("Build ");
  Serial.println(Build);
  Serial.print("ESP Name ");
  Serial.println(HOSTNAME);

  if (M5stack == true )
  {
    M5OutputBuildInfo();
  }
}

/*
 * ###################################################
 * #              START SETUP FUNCTION               #
 * ###################################################
*/

void setup() 
{
  Serial.begin(115200);

  M5.begin();

  M5.Lcd.setRotation(3);

//  Wire1.beginTransmission(0x34);
//  Wire1.write(0x12);
//  Wire1.write(0x4d); // Enable LDO2, aka OLED_VDD
//  Wire1.endTransmission();

//  M5.Lcd.writecommand(ST7735_DISPON);
//  M5.Axp.ScreenBreath(255);

  // Setup big M5 button 
  pinMode(M5ButtonPin, INPUT_PULLUP);

  OutputBuildInfo();

  SetupVL53L0X();

  Wire.begin();

  delay(3000);
  M5.Lcd.fillScreen( BLACK );
}

/*
 * ###################################################
 * #                END SETUP FUNCTION               #
 * ###################################################
*/

/*
 * ###################################################
 * #                 MAIN LOOP                       #
 * ###################################################
*/
void loop() 
{
  M5.update();
  delay(MainLoopDelay);
  CheckTOF();

  if (Trigger == true)
  {
    //M5.Lcd.setBrightness(200);
    for (int Count = SecsCountDownFrom; Count > 0; Count--)
    {
      Serial.println(Count);
      BackGround = "RED";
      //M5.Lcd.setRotation(3);
      M5.Lcd.fillScreen( RED );
      M5.Lcd.setTextColor(YELLOW);  
      M5.Lcd.setTextSize(6);
      M5.Lcd.setCursor(60, 20);
      M5.Lcd.println(Count);

      delay(990);
    }
    Serial.println("Rinse Hands");

    M5.Lcd.fillScreen( YELLOW );
    M5.Lcd.setTextColor(BLACK);  
    M5.Lcd.setTextSize(4);
    M5.Lcd.setCursor(28, 4);
    M5.Lcd.print("RINSE");
    M5.Lcd.setCursor(28, 44);
    M5.Lcd.print("HANDS");
    delay(4000);
    
    Serial.println("Well Done");
      
    M5.Lcd.fillScreen( GREEN );
    M5.Lcd.setTextColor(BLACK);  
    M5.Lcd.setTextSize(4);
    M5.Lcd.setCursor(28, 4);
    M5.Lcd.print("WELL");
    M5.Lcd.setCursor(28, 44);
    M5.Lcd.print("DONE");
    delay(2000);
    Serial.println("Ready");
    
    Trigger = false;
  }
  else
  {
    if ( BackGround != "BLACK" ) 
    {
      Serial.println("Ready");
      //M5.Lcd.setBrightness(20);
      M5.Lcd.fillScreen( BLACK );
      M5.Lcd.setTextColor(GREEN);  
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(48, 14);
      M5.Lcd.print("READY");
      BackGround = "BLACK";
    }
  }
}


/*
 * ###################################################
 * #               END MAIN LOOP                     #
 * ###################################################
*/