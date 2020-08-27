#line 1 "pump_control.ino"
                                                                               
  
                                                        
  
                                 
  
            
          
                                
                                        
  
                                         
                         
                                           
                                
  
                                                                               

#include <LiquidCrystal.h>
#include <math.h>

                                                                               
  
              
  
                                                                               

#define COUNT_4MA  194                                      
#define COUNT_20MA 981                                       
#define LOW_ERR_THRES_4_20  164                                     
#define HIGH_ERR_THRES_4_20 1000                                       
#define TANK_RANGE          5.0f                          
#define WELL_RANGE          60.0f                          
#define WATER_COLUMN        0.4327f                     
#define FULL_TANK           7.33f                              

#define MIN_STATE_TIME      60L                                                                                        

#define START_HEIGHT_PERCENT 90                                                       
#define STOP_HEIGHT_PERCENT  100                                                     
#define ALARM_HEIGHT_PERCENT 80                                                                   

#define STATE_FILL      0
#define STATE_WAIT      1
#define STATE_PUMP      2

                                                                               
  
                   
  
                                                                               

#include "Arduino.h"
int ReadAvg160(int pin);
void DisplayHeights(void);
void Display(void);
void LogState2Serial(void);
void Fault(const char *psz);
float Read_4_20(int pin);
void CalcHeights(void);
int NextState(int iState);
void setup(void);
void loop(void);
#line 53
int g_iState = STATE_FILL;

int TankPin  = 0;                                                    
int WellPin  = 1;                                                          
int PumpPin  = 11;                                              
int AlarmPin = 12;                                              

float g_fTank        = 0.0;                              
int   g_iTankPercent =   0;                                 

float g_fWell = 0.0;                                            
float g_fWB_Turnoff = 0.0;                                                      
float g_fWB_Turnon = 0.0;                                                       

long g_lTicks = 0;                                

long g_lLastStateChange = 0;

                                                                
                                            
  
LiquidCrystal lcd(10, 9, 6, 5, 4, 3);


                                                                               
  
                                                                        
                                                                        
  
              
                                                  
  
                                    
  
                                                                               

int ReadAvg160(int pin) 
{
    long lRes = 0;

    for (int i = 0; i < 160; i++) lRes += analogRead(pin);

    return lRes / 160;
}

                                                                               
  
                                                       
  
                     
  
                     
  
                                                                               

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

                                                                               
  
                     
  
                     
  
                     
  
                                                                               

void LogState2Serial(void)
{
                            
      
    Serial.print("Ticks: "); Serial.print(g_lTicks); Serial.println(" seconds. ");
    Serial.print("Current State: "); Serial.println(g_iState);
    Serial.print("Last State change @ "); Serial.println(g_lLastStateChange);
    Serial.print("Tank: "); Serial.println(g_fTank);
    Serial.print("Well: "); Serial.println(g_fWell);
    Serial.print("Well @ last pump turn on: "); Serial.println(g_fWB_Turnon);
    Serial.print("Well @ last pump turn off: "); Serial.println(g_fWB_Turnoff);
    Serial.flush();
}   

                                                                               
  
                                              
  
              
                                                                        
  
                     
  
                                                                               

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

                                                                               
  
                                                          
  
              
                                                  
  
                            
  
                                                                               

float Read_4_20(int pin)
{
    int iRes = ReadAvg160(pin);

                                                   

                                         
      
    if (iRes < LOW_ERR_THRES_4_20)  Fault("Sensor Low");
    if (iRes > HIGH_ERR_THRES_4_20) Fault("Sensor High");
    
                             
      
    if (iRes > COUNT_20MA) iRes = COUNT_20MA;
    iRes -= COUNT_4MA;
    if (iRes < 0)  iRes = 0;

    return (float) iRes / (float)(COUNT_20MA - COUNT_4MA);
}

                                                                               
  
                                                             
  
                     
  
                     
  
                                                                               

void CalcHeights(void)
{
    g_fWell = WELL_RANGE * Read_4_20(WellPin) / WATER_COLUMN;
    g_fTank = TANK_RANGE * Read_4_20(TankPin) / WATER_COLUMN;
    g_iTankPercent = (int)(g_fTank * 100.00f / FULL_TANK);
}

                                                                               
  
                                                        
  
              
                         
  
                         
  
                                                                               

int NextState(int iState)
{
                            
      
    Serial.print("New State: "); Serial.println(iState);
    LogState2Serial();
    
                                                     
      
    if (g_lLastStateChange) {
        if ((g_lTicks - g_lLastStateChange) < MIN_STATE_TIME) {
            Fault("Rapid cycling");
        }
    }
    
    g_lLastStateChange = g_lTicks;
    return iState;
}

                                                                               
  
                                                 
  
                     
  
                     
  
                                                                               

void setup(void) {
    
                                                 
  lcd.begin(20, 4);                    

                                 
  Serial.begin(9600);
  
                          
  digitalWrite(PumpPin,  LOW);
  digitalWrite(AlarmPin, LOW);
  pinMode(PumpPin, OUTPUT);         
  pinMode(AlarmPin, OUTPUT);          
  
  while (!Serial);                                    
  Serial.println("Well Pump Controller Ver 2.2\n");  
  LogState2Serial();
  
  analogReference(DEFAULT);                               
  pinMode(TankPin, INPUT);
  pinMode(WellPin, INPUT);
}

                                                                               
  
                                                  
  
                     
  
                     
  
                                                                               

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

