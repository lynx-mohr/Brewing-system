

/*Program to run Eskedalen Brewery


LCD pins: 43 38 39 40 41 42




/*Program to run Eskedalen Brewery


Task list
1. Create an array of Solenoid control relay pins
2. Set them as outputs
3. Create fluid control pins and set them as inputs and setup hardware
4. Create the wifi setup
5. Create an LCD to print info on
6. Connect temperature sensors
7. Creating an interactive menu*
8. Add RTC-clock module
9. Vill spara en brygglogg till EEPROM. För varje bryggning är detta vad jag vill spara: 

a) Datum
b) Tid för start, från när vatten börjar värmas. 
c) För varje steg sparas lite data: 
  steg 1: värmning av vatten - vad var vattentemp initialt?, vad var vald striketemp? , vad var strikevolym, hur lång tid tog det att uppnå striketemp? 
  steg 2: mäskning - vad var graintemp? Vilken temp uppnåddes efter homogenisering? hur lång tid tog det att fylla mäsktunnan? Vad var temperaturen vid mäskningens slut? 
  steg 3: lakning - vad var vattententemp initialt? hur varmt var lakvattnet när det var klart, hur lång tid tog uppvärmningen till laktemp, hur lång tid tog lakningen, från att vatten började rinna? 
  steg 4: kokning - hur varm är vörten när den kommer in i kokkärlet?, hur lång tid tar överföringen, hur lång tid tar det till kokning? 


if Program was chosen, follow this:
digitalWrite(solenoidArray[0], HIGH);
Use millis(); instead of delay() here. //the time it takes to fill the water heater tun
when 60 seconds has passed, start heater:
digitalWrite(solenoidArray[1], HIGH);
digitalRead(malt temperature from mash tun);
digitalRead(water temperature in heater):
calculate using formula what the combined temperature will land on when water volume of x temp is mixed with graintemperature of x volume and temp.


*/




#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <uRTCLib.h>


/*
rs brun 38
e orange 39
d4 gul 40
d5 blå 41
d6 rosa 42
d7 lila 43


RTC
SQW 2
*/




//defining menu item counts for the purpose of the roll around list functionality
byte ROOT_MENU_CNT = 2;
//byte SUB_MENU1_CNT = 3;
byte SUB_MENU2_CNT = 4;
byte SUB_MENU3_CNT = 20;
byte SUB_MENU4_CNT = 10;
byte SUB_MENU5_CNT = 10;
byte SUB_MENU6_CNT = 10;


//Creating variable for the Menu structure
enum pageType {ROOT_MENU, SUB_MENU1, SUB_MENU2, SUB_MENU3, SUB_MENU4, SUB_MENU5, SUB_MENU6, SUB_MENU7, SUB_MENU8, SUB_MENU9, SUB_MENU10};


//Holds which page i currently selected
enum pageType currPage = ROOT_MENU;


//Constants holding port adresses
const int BTN_ACCEPT = 16;
const int BTN_UP = 19;
const int BTN_DOWN = 18;
const int BTN_CANCEL = 15;

// Pin Definitions
const int LCD_RS = 38, LCD_EN = 39, LCD_D4 = 40, LCD_D5 = 41, LCD_D6 = 42, LCD_D7 = 43;
const int SOLENOID_PINS[] = {23, 53, 25, 27, 29, 31, 33};
const int BUTTON_LED_PINS[] = {7, 9, 6, 8, 4, 5, 3}; //7=blå vänster, 9=röd, 6=blå höger, 8=grön vä, 4=vit, 5=grön hö, 3=gul
const int BUTTON_PINS[] = {12, 14, 17, 15, 19, 18, 16}; //17=blå vä, 12=blå hö, 14=röd, 18=grön vä, 15=grön hö, 19=vit, 16=gul
const int NUM_BUTTONS = 7;
//const int FLOW_PIN2 = A9;
const int BUZZER_PIN = A0;
const int FLOWPIN_1 = 2;


#define ONE_WIRE_BUS 10 // Data wire is plugged into digital pin 10 on the Arduino


uRTCLib rtc;
//uRTCLib rtc(0x68);


char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  


// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);




// Global Variables
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
bool buttonState[NUM_BUTTONS] = {false};
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};
unsigned long debounceDelay = 200; // Adjust as needed
int mashThicknessInt; //Input from user
int mashThicknessDec; //Input from user
float mashThicknessFinal; //Input from user
int mashTempInt; //Input from user
int mashTempDec; //Input from user
float mashTempFinal; //Input from user
int grainWeightInt; //Input from user
int grainWeightDec1; //Input from user
int grainWeightDec2; //Input from user
float grainWeightFinal; //Input from user
float mashTemperature; //Input from user
//float desiredMashTemp = 65;
int waterBuffer = 10; //meaning 10 liter water buffer
float strikeTemp;
float grainTemp;
bool volumeReached = false; //keeps track of water filled in HTL
bool heatingOn = false; //keeps track of heating in HTL
bool switchedToHeatingView = false;  // flag for view change in LCD Submenu 10

float mashWaterVol;
float TEMP_HEATER; //temp measured in water heater
float TEMP_MASH; //temp measured in mash tun
float TEMP_BOIL; //temp measured in boiler
byte RELAY_PIN = SOLENOID_PINS[1];

volatile unsigned long pulseCount = 0;
float calibrationFactor = 621.0; // pulses per liter
float liters = 0.0;
float flowRate = 0.0;

unsigned long oldTime;

// --- SubMenu10 state ---
bool sm10_inited = false;              // har enter() körts?
bool sm10_showingPreset = false;       // visar vi 4s preset-rutan?
unsigned long sm10_startPresetMs = 0;  // starttid för preset-vyn
unsigned long sm10_lastUpdateMs = 0;   // senaste LCD/flow-uppdatering
unsigned long sm10_lastPulseCount = 0; // för att räkna L/min

//Flag to turn off the flow when the target volume is reached
bool filling = false; 

// ---- Target volume ----
float targetVolume; // Volume to pass to the next vessel

// ******************************************************************
// ||                           SETUP                              ||
// ******************************************************************


void setup() {


  sensors.begin();  // Start up the library (temp)

   Serial.begin(9600);

 


  // Initialize Pins
  initializePins();


  // Initialize LCD
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("ESKEDALEN");
  lcd.setCursor(0, 1);
  lcd.print("BREW SOFT 1.0");
  delay(2000);


 
for (int i = 0; i < 2; i++){




for (int i = 0; i < NUM_BUTTONS; i++) {


  digitalWrite(BUTTON_LED_PINS[i], HIGH);


  delay(100);
}


for (int i = NUM_BUTTONS -1; i >= 0; i--){


  digitalWrite(BUTTON_LED_PINS[i], LOW);


  delay(100);
}
}




//************************
 URTCLIB_WIRE.begin();


  // Comment out below line once you set the date & time.
  // Following line sets the RTC with an explicit date & time
  // for example to set January 13 2022 at 12:56 you would call:
   //rtc.set(0, 11, 9, 6, 17, 9, 24);
  // rtc.set(second, minute, hour, dayOfWeek, dayOfMonth, month(+4), year)
  // set day of week (1=Sunday, 7=Saturday)
   //*******************************


  attachInterrupt(digitalPinToInterrupt(FLOWPIN_1), flowISR, FALLING);

}


// ******************************************************************
// ||                            LOOP                              ||
// ******************************************************************


void loop() {




  switch (currPage){




  case ROOT_MENU: page_RootMenu(); break; //manual or automated brew
  case SUB_MENU1: page_SubMenu1(); break; //run manual operations
  case SUB_MENU2: page_SubMenu2(); break; //mash thickness, mash temp, grain weight
  case SUB_MENU3: page_SubMenu3(); break; //mash thickness, integer input
  case SUB_MENU4: page_SubMenu4(); break; //mash thickness, decimal input
  case SUB_MENU5: page_SubMenu5(); break; //mash temperature, integer input
  case SUB_MENU6: page_SubMenu6(); break; //mash temperature, decimal input
  case SUB_MENU7: page_SubMenu7(); break; //Grain weight, first and/or second integer input
  case SUB_MENU8: page_SubMenu8(); break; //Grain weight, first decimal input
  case SUB_MENU9: page_SubMenu9(); break; //Grain weight, second decimal input
  
     case SUB_MENU10:
      if (!sm10_inited) {            // <-- init EN gång vid inträde
        page_SubMenu10_enter();
      }

       page_SubMenu10_loop();          // <-- kör varje varv
      break;
  
  }


}


 
void initializePins() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(SOLENOID_PINS[i], OUTPUT);
    pinMode(BUTTON_LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

 pinMode(FLOWPIN_1, INPUT_PULLUP);  

/*
  pinMode(TEMP_PIN1, INPUT);
   pinMode(TEMP_PIN2, INPUT);
    pinMode(TEMP_PIN3, INPUT);
   
     pinMode(flowPin1, INPUT);
      pinMode(flowPin2, INPUT);
      pinMode(BUZZER_PIN, OUTPUT);
*/
}


void handleButtonPress() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (digitalRead(BUTTON_PINS[i]) == LOW && (millis() - lastDebounceTime[i]) > debounceDelay) {
      // Button pressed and debounce delay passed
      buttonState[i] = !buttonState[i]; // Toggle button state
      digitalWrite(BUTTON_LED_PINS[i], buttonState[i]); // Set LED state
      digitalWrite(SOLENOID_PINS[i], buttonState[i]); // Set solenoid state
      lastDebounceTime[i] = millis(); // Save the time when the button was pressed
    }
  }
}


/*
Insterrupt Service Routine
 */
 
void flowISR() {
  pulseCount++;

}




