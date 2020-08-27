//*****************************************************************************
//
// pump_control.cpp - Implementation of well pump controller.
//
// Author: Reed Bement 02/02/2016
//
//    Notes:
//    ----
//      Water: 0.4327 psi / foot
//      Full tank: 88 inches = 7.33 feet
//
//      Arduino Sketch. Arduino IDE 1.6.5
//      Arduino settings:
//        Board: Pro Trinket 5V/16MHz (USB)
//        Programmer: USBtinyISP
//
//*****************************************************************************

#include <LiquidCrystal.h>
#include <math.h>

//*****************************************************************************
//
// Definitions
//
//*****************************************************************************

#define COUNT_4MA  194              // Measured count at 4ma
#define COUNT_20MA 981              // Measured count at 20ma
#define LOW_ERR_THRES_4_20  164     // Error if less than this value
#define HIGH_ERR_THRES_4_20 1000    // Error if greater than this value
#define TANK_RANGE          5.0f    // Tank sensor:0-5 PSI
#define WELL_RANGE          60.0f   // Well sensor:0-60 PSI
#define WATER_COLUMN        0.4327f // Water, psi / foot
#define FULL_TANK           7.33f   // Full tank is 7' 4 inches

#define MIN_STATE_TIME      60L     // If we're not in a state for at least this number of seconds, something is wrong 

#define START_HEIGHT_PERCENT 90     // Pump starts when tank level drops to this value
#define STOP_HEIGHT_PERCENT  100    // Pump stops when tank level rises to this value
#define ALARM_HEIGHT_PERCENT 80     // Pump stops alarm sounds when tank level drops to this value

#define STATE_FILL      0
#define STATE_WAIT      1
#define STATE_PUMP      2

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************

int g_iState = STATE_FILL;

int TankPin  = 0;       // 4-20ma interface for tank presure, 0-5 PSI
int WellPin  = 1;       // 4-20ma interface for well bore presure, 0-60 PSI
int PumpPin  = 11;      // Solid Stae Relay (SSR), 40A, High: On
int AlarmPin = 12;      // Solid Stae Relay (SSR), 40A, High: On

float g_fTank        = 0.0;   // Tank water column height
int   g_iTankPercent =   0;   // Percetage of tank remaining

float g_fWell = 0.0;            // Well bore water column height
float g_fWB_Turnoff = 0.0;      // Well bore water column height at pump shutoff
float g_fWB_Turnon = 0.0;       // Well bore water column height at pump turn on

long g_lTicks = 0;      // One second tick counter

long g_lLastStateChange = 0;

// Initialize the library with the numbers of the interface pins
// LiquidCrystal(rs, enable, d4, d5, d6, d7)
//
LiquidCrystal lcd(10, 9, 6, 5, 4, 3);


//*****************************************************************************
//
//  ReadAvg160 - Reads the value from the specified analog pin 160 times
//               and returns the average value. Takes ~ one 60Hz period.
//
//  Parameters
//    pin - Analog input pin to read from (0 to 5)
//
//  Returns - Average value, 0-1023.
//
//*****************************************************************************

int ReadAvg160(int pin) 
{
    long lRes = 0;

    for (int i = 0; i < 160; i++) lRes += analogRead(pin);

    return lRes / 160;
}

//*****************************************************************************
//
//  DisplayHeights - Update display with new hieght values.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void DisplayHeights(void)
{
    lcd.setCursor(0, 1); lcd.print("Well: "); lcd.print(g_fWell); 
    lcd.print("'    ");
    
    lcd.setCursor(0, 2); lcd.print("Tank: "); lcd.print(g_fTank); 
    lcd.print("' (");
    lcd.print(g_iTankPercent);
    lcd.print("%)");  

    if (g_lTicks & 1) {
        lcd.setCursor(0, 3); lcd.print("Well@On: "); lcd.print(g_fWB_Turnon); 
        lcd.print("' ");
    }
    else {
        lcd.setCursor(0, 3); lcd.print("Well@Off: "); lcd.print(g_fWB_Turnoff); 
        lcd.print("'");
    }
}

//*****************************************************************************
//
//  Display - Display data on LCD.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void Display(void)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("State: ");
    switch (g_iState) {
        case STATE_FILL:
            lcd.print("Fillling");
            break;     
        case STATE_WAIT:
            lcd.print("Waiting");
            break;
        case STATE_PUMP:
            lcd.print("Pumping");
            break;
        default: lcd.print("Unknown");
    }    
    DisplayHeights();
}

//*****************************************************************************
//
//  LogState2Serial -
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void LogState2Serial(void)
{
    // Log the state change.
    //
    Serial.print("Ticks: "); Serial.print(g_lTicks); Serial.println(" seconds. ");
    Serial.print("Current State: "); Serial.println(g_iState);
    Serial.print("Last State change @ "); Serial.println(g_lLastStateChange);
    Serial.print("Tank: "); Serial.println(g_fTank);
    Serial.print("Well: "); Serial.println(g_fWell);
    Serial.print("Well @ last pump turn on: "); Serial.println(g_fWB_Turnon);
    Serial.print("Well @ last pump turn off: "); Serial.println(g_fWB_Turnoff);
    Serial.flush();
}   

//*****************************************************************************
//
//  Fault - Enter fault state. Doesn't return.
//
//  Parameters
//    psz - Fault description string. Less than 20 characters in length.
//
//  Returns - Nothing
//
//*****************************************************************************

void Fault(const char *psz)
{
    digitalWrite(PumpPin,  LOW);
    digitalWrite(AlarmPin, HIGH);

    Serial.print("Fault @ ");
    Serial.print(g_lTicks); 
    Serial.print(" seconds. ");
    Serial.println(psz);
    LogState2Serial();
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(psz);
    DisplayHeights();
    
    while (1); 
}

//*****************************************************************************
//
//  Read_4_20 - Reads the value from the 4-20ma interface.
//
//  Parameters
//    pin - Analog input pin to read from (0 to 5)
//
//  Returns - Value, 0.0-1.0
//
//*****************************************************************************

float Read_4_20(int pin)
{
    int iRes = ReadAvg160(pin);

//    Serial.print("iRes: "); Serial.println(iRes);

    // Check for open or shorted sensors.
    //
    if (iRes < LOW_ERR_THRES_4_20)  Fault("Sensor Low");
    if (iRes > HIGH_ERR_THRES_4_20) Fault("Sensor High");
    
    // Normalize to 0.0 - 1.0
    //
    if (iRes > COUNT_20MA) iRes = COUNT_20MA;
    iRes -= COUNT_4MA;
    if (iRes < 0)  iRes = 0;

    return (float) iRes / (float)(COUNT_20MA - COUNT_4MA);
}

//*****************************************************************************
//
//  CalcHeights - Calculate the heights of the water columns.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void CalcHeights(void)
{
    g_fWell = WELL_RANGE * Read_4_20(WellPin) / WATER_COLUMN;
    g_fTank = TANK_RANGE * Read_4_20(TankPin) / WATER_COLUMN;
    g_iTankPercent = (int)(g_fTank * 100.00f / FULL_TANK);
}

//*****************************************************************************
//
//  NextState - Preform state change logging and checks.
//
//  Parameters
//    iState - New state.
//
//  Returns - Next state.
//
//*****************************************************************************

int NextState(int iState)
{
    // Log the state change.
    //
    Serial.print("New State: "); Serial.println(iState);
    LogState2Serial();
    
    // Make sure we're not changing state too rapidly
    //
    if (g_lLastStateChange) {
        if ((g_lTicks - g_lLastStateChange) < MIN_STATE_TIME) {
            Fault("Rapid cycling");
        }
    }
    
    g_lLastStateChange = g_lTicks;
    return iState;
}

//*****************************************************************************
//
//  setup - Arduino entry point. Called one time.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void setup(void) {
    
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);  // 20x4 Characters

  // Initialize debug serial port
  Serial.begin(9600);
  
  // Setup SSR output pins
  digitalWrite(PumpPin,  LOW);
  digitalWrite(AlarmPin, LOW);
  pinMode(PumpPin, OUTPUT);  // Pump
  pinMode(AlarmPin, OUTPUT);  // Alarm
  
  while (!Serial); // wait for Serial port to connect.
  Serial.println("Well Pump Controller Ver 2.2\n");  
  LogState2Serial();
  
  analogReference(DEFAULT); // Analog reference of 5 volts
  pinMode(TankPin, INPUT);
  pinMode(WellPin, INPUT);
}

//*****************************************************************************
//
//  loop - Arduino entry point. Called repeatedly.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void loop(void) {

    Display();
    delay(1000);

    CalcHeights();
    if (g_iState != STATE_FILL) {
        if (g_iTankPercent < ALARM_HEIGHT_PERCENT) Fault("Low Tank Level");
    }
    
    switch (g_iState) {
        case STATE_FILL:
            if (g_iTankPercent >= STOP_HEIGHT_PERCENT) {
                g_fWB_Turnoff = g_fWell;
                g_iState = NextState(STATE_WAIT);
            }
            else digitalWrite(PumpPin, HIGH);
            break;
                
        case STATE_WAIT:
            if (g_iTankPercent < START_HEIGHT_PERCENT) {
                g_fWB_Turnon = g_fWell;
                g_iState = NextState(STATE_PUMP);
            }
            else digitalWrite(PumpPin, LOW);
            break;
            
        case STATE_PUMP:
            if (g_iTankPercent >= STOP_HEIGHT_PERCENT) {
                g_fWB_Turnoff = g_fWell;
                g_iState = NextState(STATE_WAIT);
            }
            else digitalWrite(PumpPin, HIGH);
            break;
            
       default: Fault("Bad State");
    }
    g_lTicks++;
}