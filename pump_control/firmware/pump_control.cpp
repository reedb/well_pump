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
//      Possible faults:
//        "Sensor Low"     - Control problem, sensor out of range.
//        "Sensor High"    - Control problem, sensor out of range.
//        "Rapid Cycling"  - Control problem, attempting to switch the pump on/off too quickly.
//        "Bad State"      - Control problem, unknow software state.
//
//        "Low Tank Level" - Tank level below threshold (only in STATE_PUMP).
//        "Low Well Level" - Well level below threshold (only in STATE_PUMP).
//        "Pump Long Run"  - Pump run time above threshold (only in STATE_PUMP).
//         
//*****************************************************************************

#include <LiquidCrystal.h>
#include <math.h>
#include <util/atomic.h>

//*****************************************************************************
//
// Definitions
//
//***************************************************************************** 

#define VER_STRING          "\n\nWell Pump Controller Ver 2.31\n"
#define LCD_SPLASH_STRING   "Well Pump Controller"
#define LCD_VER_STRING      "Version: 2.31"
                             
#define LOG_INTERVAL        60      // How often logging occurs [s]

#define COUNT_4MA           194     // Measured count at 4ma
#define COUNT_20MA          981     // Measured count at 20ma
#define LOW_ERR_THRES_4_20  164     // Error if less than this value
#define HIGH_ERR_THRES_4_20 1000    // Error if greater than this value
#define TANK_RANGE          5.0f    // Tank sensor:0-5 PSI
#define WELL_RANGE          60.0f   // Well sensor:0-60 PSI
#define WATER_COLUMN        0.4327f // Water Column (WC) presure to height, psi / foot
#define FULL_TANK           7.33f   // Full tank is 7' 4 inches

#define MIN_PUMP_TIME       60L     // If we're not pumping/non-pumping for at least this number of seconds, something is wrong 
#define MAX_PUMP_TIME       21600L  // If we're in the PUMP state for at least this number of seconds (6 hours), something is wrong
#define FILL_WAIT_TIME      3600L   // Time to spent in the FILL_WAIT state [s]

#define START_HEIGHT_PERCENT 90     // Pump starts when tank level drops to this value
#define STOP_HEIGHT_PERCENT  100    // Pump stops when tank level rises to this value
#define ALARM_HEIGHT_PERCENT 80     // Pump stops alarm sounds when tank level drops to this value

#define STOP_WELL_LEVEL_FEET  20.0  // Pump stops when well level drops to this value. [ft]

#define STATE_FILL      0
#define STATE_FILL_WAIT 1
#define STATE_WAIT      2
#define STATE_PUMP      3

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************

int g_iState = STATE_FILL;

int TankPin  = 0;       // 4-20ma interface for tank presure, 0-5 PSI
int WellPin  = 1;       // 4-20ma interface for well bore presure, 0-60 PSI
int PumpPin  = 11;      // Solid Stae Relay (SSR), 40A, High: Pump On
int AlarmPin = 12;      // Solid Stae Relay (SSR), 40A, High: Alarm On
int LEDPin   = 13;      // Red user status LED mounted on ProTrinket board.

float g_fTank        = 0.0;   // Tank water column height [ft]
int   g_iTankPercent =   0;   // Percetage of tank remaining

float g_fWell = 0.0;          // Well bore water column height [ft]
float g_fWB_Turnoff = 0.0;    // Well bore water column height at pump shutoff [ft]
float g_fWB_Turnon = 0.0;     // Well bore water column height at pump turn on [ft] 

unsigned long g_ulPumpLastStart   = 0; // Last time the pump started [s]
unsigned long g_ulPumpLastStop    = 0; // Last time the pump stopped [s]

volatile unsigned long g_ulTicks = 0;     // One second tick counter, updated by timer1 ISR

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

    if (GetTickCount() & 1) {
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
//  State2Str - Convert state to string.
//
//  Parameters
//      iState - State to convert to string.
//
//  Returns - String representation of passed state.
//
//*****************************************************************************

const char *State2Str(int iState)
{
    switch (iState) {
        case STATE_FILL: return("Fill");
        case STATE_FILL_WAIT: return("Fill Wait");
        case STATE_WAIT: return("Wait");
        case STATE_PUMP: return("Pump");
        default: return("Unknown");
    }    
}

//*****************************************************************************
//
//  Display - Display data on LCD. Display is 20 x 4 ASCII Characters.
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
    lcd.print(State2Str(g_iState));
    if ((g_iState == STATE_PUMP) || (g_iState == STATE_FILL)) {
        lcd.print(" ");  
        lcd.print(TimeSince(g_ulPumpLastStart) / 60);
        lcd.print("min");  
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
    Serial.println("");
    Serial.print("Ticks: "); Serial.print(GetTickCount()); Serial.println(" seconds. ");
    Serial.print("State: "); Serial.print(State2Str(g_iState));
    if ((g_iState == STATE_PUMP) || (g_iState == STATE_FILL)) { 
        Serial.print(" ");  
        Serial.print(TimeSince(g_ulPumpLastStart) / 60);
        Serial.println("min");  
    }
    else Serial.println("");
    Serial.print("Tank: "); Serial.print(g_fTank);Serial.print("' (");
    Serial.print(g_iTankPercent);Serial.println("%)");  
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

    Serial.println("");
    Serial.print("Fault @ ");
    Serial.print(GetTickCount()); 
    Serial.print(" seconds!!!!!!!!!!!!!");
    Serial.println(psz);
    Serial.println(VER_STRING);
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
//  PumpOn, PumpOff - Turn the pump on/off, but only toggle.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

void PumpOn(void)
{  
    if (digitalRead(PumpPin) == 0) {
        g_fWB_Turnon = g_fWell;
        g_ulPumpLastStart = GetTickCount();
        if (g_ulPumpLastStart - g_ulPumpLastStop < MIN_PUMP_TIME) Fault("Rapid Cycling");
        digitalWrite(PumpPin, HIGH);
    }
}

void PumpOff(void)
{
   if (digitalRead(PumpPin) == 1) {
       g_fWB_Turnoff = g_fWell;
       g_ulPumpLastStop = GetTickCount();
       if (g_ulPumpLastStop - g_ulPumpLastStart < MIN_PUMP_TIME) Fault("Rapid Cycling");
       digitalWrite(PumpPin, LOW); 
   }
}

//*****************************************************************************
//
//  NextState - Preform state change logging.
//
//  Parameters
//    iState - New state.
//
//  Returns - Next state.
//
//*****************************************************************************

int NextState(int iState)
{   
    LogState2Serial();
    Serial.print("\nNew State: "); Serial.println(State2Str(iState));
    return iState;
}

//*****************************************************************************
//
//  GetTickCount - Get the tick count since start up.
//
//  Parameters - None
//
//  Returns - Tick count in seconds.
//
//*****************************************************************************

unsigned long GetTickCount(void)
{   
    unsigned long ulTicks;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ulTicks = g_ulTicks;
    }
    return ulTicks;
}

//*****************************************************************************
//
//  TimeSince - Get the tick count since event occured.
//
//  Parameters 
//    ulEventTime - Event time in seconds.
//
//  Returns - Tick count in seconds since event occured.
//
//*****************************************************************************

unsigned long TimeSince(unsigned long ulEventTime)
{   
    return GetTickCount() - ulEventTime;
}


//*****************************************************************************
//
//  TIMER1_COMPA_vect - Inerupt service routine, updates 1Hz tick count.
//
//  Parameters - None
//
//  Returns - Nothing
//
//*****************************************************************************

ISR(TIMER1_COMPA_vect)
{
    g_ulTicks++;
    if (g_ulTicks & 1) digitalWrite(LEDPin, HIGH);
    else  digitalWrite(LEDPin, LOW);
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

  // Set timer1 to interrupt at 1Hz
  //
  cli();                // stop interrupts
  TCCR1A = 0;           // set entire TCCR1A register to 0
  TCCR1B = 0;           // same for TCCR1B
  TCNT1  = 0;           // initialize counter value to 0
  OCR1A = 15624;        // = (16*10^6) / (1*1024) - 1 (must be < 65536)
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  TIMSK1 |= (1 << OCIE1A);
  sei();                //allow interrupts

  // Initialize debug serial port
  Serial.begin(9600);
  
  // Setup SSR output pins
  digitalWrite(PumpPin,  LOW);
  digitalWrite(AlarmPin, LOW);
  pinMode(PumpPin,  OUTPUT);    // Pump
  pinMode(AlarmPin, OUTPUT);    // Alarm
  pinMode(LEDPin,   OUTPUT);    // Status LED
  
  while (!Serial); // wait for Serial port to connect.
  Serial.println();  
  Serial.println(VER_STRING);
  
  analogReference(DEFAULT); // Analog reference of 5 volts
  pinMode(TankPin, INPUT);
  pinMode(WellPin, INPUT); 
    
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);  // 20x4 Characters
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(LCD_SPLASH_STRING);
  lcd.setCursor(0, 1); lcd.print(LCD_VER_STRING);
  delay(4000);
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
    static unsigned long ulLastTick = 0;
    
    // Loop to run once per second.
    while (ulLastTick == GetTickCount());
    ulLastTick = GetTickCount();

    CalcHeights();
    
    switch (g_iState) {
        case STATE_FILL:
            if (g_iTankPercent >= STOP_HEIGHT_PERCENT) {
                g_iState = NextState(STATE_WAIT);
            }
            else if (g_fWell < STOP_WELL_LEVEL_FEET) {
                g_iState = NextState(STATE_FILL_WAIT);
            }
            else PumpOn();
            break;
        
        case STATE_FILL_WAIT:
            PumpOff();
            if (TimeSince(g_ulPumpLastStop) > FILL_WAIT_TIME) {
                g_iState = NextState(STATE_FILL);
            }
            break;
            
        case STATE_WAIT:
            if (g_iTankPercent < START_HEIGHT_PERCENT) {
                g_iState = NextState(STATE_PUMP);
            }
            else PumpOff();
            break;
            
        case STATE_PUMP:  
            if (g_fWell < STOP_WELL_LEVEL_FEET)         Fault("Low Well Level");
            if (g_iTankPercent < ALARM_HEIGHT_PERCENT)  Fault("Low Tank Level");
            if (g_iTankPercent >= STOP_HEIGHT_PERCENT) {
                g_iState = NextState(STATE_WAIT);
            }
            else PumpOn();
            if (TimeSince(g_ulPumpLastStart) > MAX_PUMP_TIME) Fault("Pump Long Run");
            break;
            
        default:
            Fault("Bad State");
    }

    if ((GetTickCount() % LOG_INTERVAL) == 0) LogState2Serial();
    Display();
}
