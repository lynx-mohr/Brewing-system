/* * ESKEDALEN BREW SOFT 1.9.4
 * Status: Ljusshow återställd, Nomenclature fixad, Sparge-auto inkluderad.
 */

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- PIN-DEFINITIONER ---
const int LCD_RS = 38, LCD_EN = 39, LCD_D4 = 40, LCD_D5 = 41, LCD_D6 = 42, LCD_D7 = 43;
/*Solenoid/button: 
Blue, left, water in = SOLENOID_PINS[0] = pin 23                (säker)
Red, heat on = SOLENOID_PINS[1] = pin 53                        (säker)
Blue, right, cooling water = SOLENOID_PINS[2] = pin 25          (gissar)
Yellow, strike water out = SOLENOID_PINS[6] = pin 33            (gissar)
White, lautering pin = SOLENOID_PINS[4] = pin 29                (gissar)
Green, left, water to boiler = SOLENOID_PINS[3] = 27            (gissar)
Green, right, wort to fermenter = SOLENOID_PINS[5] = 31         (gissar)
*/

const int SOLENOID_PINS[] = {23, 53, 25, 27, 29, 31, 33}; 
const int BUTTON_LED_PINS[] = {7, 9, 6, 8, 4, 5, 11}; 
const int BUTTON_PINS[] = {12, 14, 17, 15, 19, 18, 16};
const int NUM_BUTTONS = 7;

const int FLOWPIN_IN = 2;   
const int FLOWPIN_OUT = 21; 
const int ONE_WIRE_BUS = 10;
const int BUZZER_PIN = A0; 

#define VALVE_COLD_IN  SOLENOID_PINS[0]
#define VALVE_STRIKE_OUT SOLENOID_PINS[6]
#define RELAY_PIN SOLENOID_PINS[1]
#define LAUTER_PIN SOLENOID_PINS[4]
#define COOLING_WATER SOLENOID_PINS[2]
#define WORT_TO_BOILER SOLENOID_PINS[3]
#define WORT_TO_FERMENTER SOLENOID_PINS[5]


#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_C6  1047

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- GLOBALA VARIABLER & KONSTANTER ---
enum pageType {SUB_MENU2, SUB_MENU3, SUB_MENU4, SUB_MENU5, SUB_MENU6, SUB_MENU7, SUB_MENU8, SUB_MENU9, SUB_MENU10, SUB_MENU11, SUB_MENU12, SUB_MENU13, SUB_MENU_BATCH, SUB_MENU_LAUTER};
pageType currPage = SUB_MENU2;

int mashThicknessInt = 3, mashThicknessDec = 0;
int mashTempInt = 65, mashTempDec = 0;
int grainWeightInt = 15, grainWeightDec1 = 0, grainWeightDec2 = 0;
int batchVolInt = 60; 
int mashTime = 60; 

float targetVolume, strikeTemp, spargeVolume, liters = 0, grainTemp = 20.0;
const float calibrationFactor = 450.0;
const float ABSORPTION_RATE = 1.04; 
const float BOIL_OFF_PCT = 1.15;    
const float EQUIP_LOSS = 2.0;       
const float HLT_DEAD_SPACE = 10.0; // Liter extra som alltid ska finnas kvar i HLT

volatile unsigned long pulseCountIn = 0;
volatile unsigned long pulseCountOut = 0;

bool lastOkState = HIGH; 
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};
unsigned long emergencyStart = 0;
bool sm10_inited = false, sm10_showingPreset = false, volumeReached = false, transferring = false;
bool spargeInitiated = false;
unsigned long sm10_timer = 0;

void flowISR_In() { pulseCountIn++; }
void flowISR_Out() { pulseCountOut++; }

// --- HJÄLPFUNKTIONER ---
bool btnIsDown(int pin) { return digitalRead(pin) == LOW; }

bool btnWasClicked(int pin) {
    bool currentState = digitalRead(pin);
    if (currentState == LOW && lastOkState == HIGH) {
        tone(BUZZER_PIN, 2000, 30); 
        delay(150); 
        lastOkState = LOW;
        return true;
    }
    if (currentState == HIGH) lastOkState = HIGH;
    return false;
}

void printSelected(uint8_t p1, uint8_t p2) { lcd.print(p1 == p2 ? "->" : "  "); }

void stopAllOperations() {
    tone(BUZZER_PIN, 400, 500); 
    for (int i = 0; i < NUM_BUTTONS; i++) {
        digitalWrite(SOLENOID_PINS[i], LOW);
        digitalWrite(BUTTON_LED_PINS[i], LOW);
    }
}

// ******************************************************************
// ||                            SETUP                             ||
// ******************************************************************
void setup() {
    sensors.begin();
    lcd.begin(20, 4);
    pinMode(BUZZER_PIN, OUTPUT);
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(SOLENOID_PINS[i], OUTPUT);
        pinMode(BUTTON_LED_PINS[i], OUTPUT);
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }
    pinMode(FLOWPIN_IN, INPUT_PULLUP);
    pinMode(FLOWPIN_OUT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOWPIN_IN), flowISR_In, FALLING);
    attachInterrupt(digitalPinToInterrupt(FLOWPIN_OUT), flowISR_Out, FALLING);
    
    // --- LJUSSHOW ÅTERSTÄLLD ---
    lcd.setCursor(0, 0); lcd.print("ESKEDALEN");
    lcd.setCursor(0, 1); lcd.print("BREW SOFT 1.8.8");

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < NUM_BUTTONS; i++) {
            digitalWrite(BUTTON_LED_PINS[i], HIGH);
            delay(100);
        }
        for (int i = NUM_BUTTONS - 1; i >= 0; i--) {
            digitalWrite(BUTTON_LED_PINS[i], LOW);
            delay(100);
        }
    }
    lcd.clear();
}

// ******************************************************************
// ||                            LOOP                              ||
// ******************************************************************


void loop() {
    // --- MANUELL VENTILSTYRNING (FILTER) ---
    // Vi tillåter manuell kontroll endast under de aktiva bryggstegen.
    // Detta förhindrar att solenoiderna smattrar när man navigerar i menyerna.
    if (currPage == SUB_MENU10 ||        // Automation (Fyllning, Värme, Transfer)
        currPage == SUB_MENU12 ||        // Val av mäsktid
        currPage == SUB_MENU11 ||        // Mäsktimern (här sker även Sparge)
        currPage == SUB_MENU_LAUTER) {   // Lakningsfasen
        
        handleManualValves(); 
    }
    
    // --- SID-NAVIGERING ---
    switch (currPage) {
        case SUB_MENU2:      page_InputOverview();      break; 
        case SUB_MENU3:      input_MashThicknessInt();  break;
        case SUB_MENU4:      input_MashThicknessDec();  break;
        case SUB_MENU5:      input_MashTempInt();       break;
        case SUB_MENU6:      input_MashTempDec();       break;
        case SUB_MENU7:      input_GrainWeightInt();    break;
        case SUB_MENU8:      input_GrainWeightDec1();   break;
        case SUB_MENU9:      input_GrainWeightDec2();   break;
        case SUB_MENU_BATCH: input_BatchVolume();       break;
        
        case SUB_MENU10: 
            if (!sm10_inited) page_SubMenu10_enter();
            page_SubMenu10_loop();
            break;
            
        case SUB_MENU12:     page_InputMashTime();      break;
        case SUB_MENU11:     page_MashTimer();          break;
        case SUB_MENU_LAUTER: page_Lautering();          break;
        case SUB_MENU13:     page_RunConfirm();         break;
    }
}

// ******************************************************************
// ||                        MENY-LOGIK                            ||
// ******************************************************************

void page_InputOverview() {
    static uint8_t sub_Pos = 1;
    lcd.setCursor(0,0); printSelected(1, sub_Pos); lcd.print("Mash thickn: "); lcd.print(mashThicknessInt); lcd.print("."); lcd.print(mashThicknessDec);
    lcd.setCursor(0,1); printSelected(2, sub_Pos); lcd.print("Mash temp: "); lcd.print(mashTempInt); lcd.print("."); lcd.print(mashTempDec);
    lcd.setCursor(0,2); printSelected(3, sub_Pos); lcd.print("Grain wt: "); lcd.print(grainWeightInt); lcd.print("."); lcd.print(grainWeightDec1);
    lcd.setCursor(0,3); printSelected(4, sub_Pos); lcd.print("Batch V: "); lcd.print(batchVolInt); lcd.print(" L");

    if (btnIsDown(15) && sub_Pos < 4) { sub_Pos++; tone(BUZZER_PIN, 2000, 20); delay(200); }
    if (btnIsDown(19) && sub_Pos > 1) { sub_Pos--; tone(BUZZER_PIN, 2000, 20); delay(200); }
    
    if (btnWasClicked(18)) {
        if(sub_Pos == 1) currPage = SUB_MENU3;
        else if(sub_Pos == 2) currPage = SUB_MENU5;
        else if(sub_Pos == 3) currPage = SUB_MENU7;
        else if(sub_Pos == 4) currPage = SUB_MENU_BATCH; 
        lcd.clear();
    }
}

void input_BatchVolume() {
    lcd.setCursor(0,0); lcd.print("SET BATCH VOLUME"); 
    lcd.setCursor(0,2); lcd.print("-> "); lcd.print(batchVolInt); lcd.print(" L");
    if (btnIsDown(15)) { batchVolInt++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && batchVolInt > 1) { batchVolInt--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU13; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU2; lcd.clear(); delay(200); }
}

void input_MashThicknessInt() {
    lcd.setCursor(0,0); lcd.print("Mash thickness"); 
    lcd.setCursor(0,2); lcd.print("-> "); lcd.print(mashThicknessInt); lcd.print(" , "); lcd.print(mashThicknessDec);
    if (btnIsDown(15)) { mashThicknessInt++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && mashThicknessInt > 1) { mashThicknessInt--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU4; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU2; lcd.clear(); delay(200); }
}

void input_MashThicknessDec() {
    lcd.setCursor(0,0); lcd.print("Mash thickness"); 
    lcd.setCursor(0,2); lcd.print("   "); lcd.print(mashThicknessInt); lcd.print(" , "); lcd.print("-> "); lcd.print(mashThicknessDec);
    if (btnIsDown(15) && mashThicknessDec < 9) { mashThicknessDec++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && mashThicknessDec > 0) { mashThicknessDec--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU2; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU3; lcd.clear(); delay(200); }
}

void input_MashTempInt() {
    lcd.setCursor(0,0); lcd.print("Mash temperature"); 
    lcd.setCursor(0,2); lcd.print("-> "); lcd.print(mashTempInt); lcd.print(" , "); lcd.print(mashTempDec);
    if (btnIsDown(15)) { mashTempInt++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && mashTempInt > 20) { mashTempInt--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU6; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU2; lcd.clear(); delay(200); }
}

void input_MashTempDec() {
    lcd.setCursor(0,0); lcd.print("Mash temperature"); 
    lcd.setCursor(0,2); lcd.print("   "); lcd.print(mashTempInt); lcd.print(" , "); lcd.print("-> "); lcd.print(mashTempDec);
    if (btnIsDown(15) && mashTempDec < 9) { mashTempDec++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && mashTempDec > 0) { mashTempDec--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU2; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU5; lcd.clear(); delay(200); }
}

void input_GrainWeightInt() {
    lcd.setCursor(0,0); lcd.print("Grain weight"); 
    lcd.setCursor(0,2); lcd.print("-> "); lcd.print(grainWeightInt); lcd.print(" , "); lcd.print(grainWeightDec1);
    if (btnIsDown(15)) { grainWeightInt++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && grainWeightInt > 1) { grainWeightInt--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU8; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU2; lcd.clear(); delay(200); }
}

void input_GrainWeightDec1() {
    lcd.setCursor(0,0); lcd.print("Grain weight"); 
    lcd.setCursor(0,2); lcd.print("   "); lcd.print(grainWeightInt); lcd.print(" , "); lcd.print("-> "); lcd.print(grainWeightDec1);
    if (btnIsDown(15) && grainWeightDec1 < 9) { grainWeightDec1++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && grainWeightDec1 > 0) { grainWeightDec1--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU9; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU7; lcd.clear(); delay(200); }
}

void input_GrainWeightDec2() {
    lcd.setCursor(0,0); lcd.print("Grain weight"); 
    lcd.setCursor(0,2); lcd.print("   "); lcd.print(grainWeightInt); lcd.print(" , "); lcd.print(grainWeightDec1); lcd.print("-> "); lcd.print(grainWeightDec2);
    if (btnIsDown(15) && grainWeightDec2 < 9) { grainWeightDec2++; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnIsDown(19) && grainWeightDec2 > 0) { grainWeightDec2--; tone(BUZZER_PIN, 2000, 20); delay(200); } 
    if (btnWasClicked(18)) { currPage = SUB_MENU2; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU8; lcd.clear(); delay(200); }
}

void page_InputMashTime() {
    lcd.setCursor(0,0); lcd.print("SET MASH TIME:");
    lcd.setCursor(0, 2); 
    if (mashTime == 60) lcd.print(">60 MIN<   90 MIN ");
    else                lcd.print(" 60 MIN   >90 MIN<");

    if (btnIsDown(15) || btnIsDown(19)) {
        mashTime = (mashTime == 60) ? 90 : 60;
        tone(BUZZER_PIN, 2000, 20); delay(250); 
    }
    if (btnWasClicked(18)) { currPage = SUB_MENU11; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU2; lcd.clear(); }
}

void page_RunConfirm() {
    lcd.setCursor(0,0); lcd.print("   RUN PROGRAM?   ");
    lcd.setCursor(0,2); lcd.print("OK     = START");
    lcd.setCursor(0,3); lcd.print("CANCEL = BACK");
    if (btnWasClicked(18)) { currPage = SUB_MENU10; lcd.clear(); }
    if (btnIsDown(16)) { currPage = SUB_MENU2; lcd.clear(); }
}

// ******************************************************************
// ||                    SUB_MENU 10 - AUTOMATION                  ||
// ******************************************************************

void page_SubMenu10_enter() {
    float mT = (float)mashThicknessInt + ((float)mashThicknessDec * 0.1);
    float mTemp = (float)mashTempInt + ((float)mashTempDec * 0.1);
    float gW = (float)grainWeightInt + ((float)grainWeightDec1 * 0.1) + ((float)grainWeightDec2 * 0.01);
    float bV = (float)batchVolInt;

    strikeTemp = (0.41 / mT) * (mTemp - grainTemp) + mTemp;
   targetVolume = (mT * gW) + HLT_DEAD_SPACE;

    float grainAbsorp = gW * ABSORPTION_RATE;
    float totalWaterNeeded = (bV * BOIL_OFF_PCT) + EQUIP_LOSS + grainAbsorp;
    spargeVolume = totalWaterNeeded - targetVolume;
    if (spargeVolume < 0) spargeVolume = 0;

    pulseCountIn = 0; pulseCountOut = 0;
    volumeReached = false; transferring = false; spargeInitiated = false;
    sm10_showingPreset = true; sm10_timer = millis(); sm10_inited = true;
    
    digitalWrite(VALVE_COLD_IN, HIGH); 
    digitalWrite(BUTTON_LED_PINS[0], HIGH); 
    lcd.clear();
}

void page_SubMenu10_loop() {
    unsigned long now = millis();
    sensors.requestTemperatures();
    float tHLT = sensors.getTempCByIndex(0);
    
    // Emergency stop (hålla in båda knapparna)
    if (btnIsDown(15) && btnIsDown(19)) {
        if (emergencyStart == 0) emergencyStart = now;
        if (now - emergencyStart > 2000) { 
            stopAllOperations(); currPage = SUB_MENU2; sm10_inited = false;
            lcd.clear(); lcd.print("EMERGENCY STOP!"); delay(2000); lcd.clear(); return;
        }
    } else { emergencyStart = 0; }

    // Visa presets i början
    if (sm10_showingPreset) {
        lcd.setCursor(0,0); lcd.print("MASH VOL: "); lcd.print(targetVolume - HLT_DEAD_SPACE, 1);
        lcd.setCursor(0,1); lcd.print("HLT FILL: "); lcd.print(targetVolume, 1);
        lcd.setCursor(0,2); lcd.print("STRIKE T: "); lcd.print(strikeTemp, 1);
        if (now - sm10_timer > 4000) { sm10_showingPreset = false; lcd.clear(); }
        return;
    }

    if (!volumeReached) {
        // --- FAS 1 & 2: SAMTIDIG FYLLNING OCH VÄRMNING ---
        noInterrupts(); unsigned long pIn = pulseCountIn; interrupts();
        liters = (float)pIn / calibrationFactor;
        
        lcd.setCursor(0,0); lcd.print("1. FILLING & HEAT");
        lcd.setCursor(0,1); lcd.print(liters, 1); lcd.print("/"); lcd.print(targetVolume, 1); lcd.print(" L");
        lcd.setCursor(0,2); lcd.print("TEMP: "); lcd.print(tHLT, 1); lcd.print("/"); lcd.print(strikeTemp, 1);

        // STARTA VÄRME TIDIGT: Så fort elementet är täckt (Dead Space uppnått)
        if (liters >= HLT_DEAD_SPACE) {
            if (tHLT < strikeTemp) {
                digitalWrite(RELAY_PIN, HIGH);
                digitalWrite(BUTTON_LED_PINS[1], HIGH);
                lcd.setCursor(15, 2); lcd.print("HEAT");
            } else {
                digitalWrite(RELAY_PIN, LOW);
                digitalWrite(BUTTON_LED_PINS[1], LOW);
                lcd.setCursor(15, 2); lcd.print("OK  ");
            }
        }

        // KOLLA OM FYLLNING ÄR KLAR
        if (liters >= targetVolume) { 
            digitalWrite(VALVE_COLD_IN, LOW); 
            digitalWrite(BUTTON_LED_PINS[0], LOW);
            volumeReached = true; 
            tone(BUZZER_PIN, 1000, 200); 
            // Vi hoppar inte till nästa fas förrän temperaturen OCKSÅ är klar
        }
    } 
    else if (!transferring) {
        // --- FAS 2 FORTLÖPER: VÄNTA PÅ SLUTTEMPERATUR (om fyllningen blev klar först) ---
        lcd.setCursor(0,0); lcd.print("2. FINAL HEATING");
        lcd.setCursor(0,1); lcd.print("VOL READY: "); lcd.print(liters, 1);
        lcd.setCursor(0,2); lcd.print("TEMP: "); lcd.print(tHLT, 1); lcd.print("/"); lcd.print(strikeTemp, 1);
        
        if (tHLT >= strikeTemp) { 
            digitalWrite(RELAY_PIN, LOW); 
            lcd.setCursor(0,3); lcd.print("READY! PRESS OK ");
            if (btnWasClicked(18)) { 
                transferring = true; 
                noInterrupts(); pulseCountOut = 0; interrupts(); // Nollställ ut-mätare
                lcd.clear(); 
            }
        } else { 
            digitalWrite(RELAY_PIN, HIGH); 
            digitalWrite(BUTTON_LED_PINS[1], HIGH);
            lcd.setCursor(15, 2); lcd.print("HEAT");
        }
    } 
    else {
        // --- FAS 3: ÖVERFÖRING (TRANSFER) ---
        float amountToTransfer = targetVolume - HLT_DEAD_SPACE;
        digitalWrite(VALVE_STRIKE_OUT, HIGH);
        digitalWrite(BUTTON_LED_PINS[6], HIGH);
        
        float tMash = sensors.getTempCByIndex(1);
        noInterrupts(); unsigned long pOut = pulseCountOut; interrupts();
        float lOut = (float)pOut / calibrationFactor;

        lcd.setCursor(0,0); lcd.print("3. TRANSFERRING ");
        lcd.setCursor(0,1); lcd.print("SENT: "); lcd.print(lOut, 1); lcd.print("/"); lcd.print(amountToTransfer, 1);
        lcd.setCursor(0,2); lcd.print("HLT T: "); lcd.print(tHLT, 1);
        lcd.setCursor(0,3); lcd.print("MSH T: "); lcd.print(tMash, 1);

        if (lOut >= amountToTransfer || btnWasClicked(18)) { 
            stopAllOperations(); 
            currPage = SUB_MENU12; 
            sm10_inited = false; 
            lcd.clear(); 
            lcd.print("TRANSFER COMPLETE!"); 
            delay(2000); lcd.clear();
        }
    }
}
void page_MashTimer() {
    static bool timerRunning = false;
    static bool mashingDone = false; 
    static unsigned long startTime = 0;
    unsigned long duration = (unsigned long)mashTime * 1ULL * 1000ULL;
    unsigned long now = millis();

    if (!timerRunning && !mashingDone) {
        // --- VÄNTAR PÅ START ---
        lcd.setCursor(0, 0); lcd.print("MASH IN & STIR!    ");
        lcd.setCursor(0, 2); lcd.print("PRESS OK TO START  ");
        lcd.setCursor(0, 3); lcd.print("TIMER: "); lcd.print(mashTime); lcd.print(" MIN    ");
        
        if (btnWasClicked(18)) { 
            startTime = now; 
            timerRunning = true; 
            spargeInitiated = false; // Nollställ inför ny körning
            lcd.clear(); 
        }
    } 
    else if (timerRunning) {
        // --- TIMERN TICKAR ---
        unsigned long elapsed = now - startTime;
        unsigned long remaining = (duration - elapsed) / 1000;
        int mins = remaining / 60;
        int secs = remaining % 60;

        sensors.requestTemperatures();
        float tHLT = sensors.getTempCByIndex(0);
        float tMash = sensors.getTempCByIndex(1);

        // 1. AUTO-SPARGE TRIGGER (vid 20 min kvar)
        if (mins < 20 && !spargeInitiated) {
            noInterrupts(); pulseCountIn = 0; interrupts();
            digitalWrite(VALVE_COLD_IN, HIGH);
            digitalWrite(BUTTON_LED_PINS[0], HIGH);
            spargeInitiated = true;
        }

        // 2. RAD 0: STATUS & TID
        lcd.setCursor(0, 0); lcd.print("MASHING: ");
        if(mins<10) lcd.print("0"); lcd.print(mins);
        lcd.print(":");
        if(secs<10) lcd.print("0"); lcd.print(secs);
        lcd.print("    ");

        // 3. RAD 1: SPARGE INFO (Visas endast om aktiv eller klar)
        if (spargeInitiated) {
            noInterrupts(); unsigned long pIn = pulseCountIn; interrupts();
            float sLiters = (float)pIn / calibrationFactor;
            
            lcd.setCursor(0, 1);
            lcd.print("SPRGE:"); lcd.print(sLiters, 1); lcd.print("/"); lcd.print(spargeVolume, 1); lcd.print("L ");

            if (sLiters >= spargeVolume && digitalRead(VALVE_COLD_IN) == HIGH) {
                digitalWrite(VALVE_COLD_IN, LOW); 
                digitalWrite(BUTTON_LED_PINS[0], LOW);
            }

            // Värmestyrning vid > 20 liter
            if (sLiters >= 20.0) {
                lcd.setCursor(15, 2);
                if (tHLT < 78.0) { digitalWrite(RELAY_PIN, HIGH); lcd.print("HEAT"); }
                else { digitalWrite(RELAY_PIN, LOW); lcd.print("DONE"); }
            }
        } else {
            // Rensa raden om vi inte börjat sparge än
            lcd.setCursor(0, 1); lcd.print("                    ");
        }

        // 4. RAD 2 & 3: TEMPERATURER
        lcd.setCursor(0, 2); lcd.print("HLT T: "); lcd.print(tHLT, 1); lcd.print(" C ");
        lcd.setCursor(0, 3); lcd.print("MSH T: "); lcd.print(tMash, 1); lcd.print(" C ");

        if (elapsed >= duration) {
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(VALVE_COLD_IN, LOW);
            playMashingCompleteMelody(); 
            timerRunning = false;
            mashingDone = true;
            lcd.clear();
        }
    }
    else if (mashingDone) {
        // --- KLAR - VÄNTAR PÅ LAKNING ---
        lcd.setCursor(0, 0); lcd.print("MASHING COMPLETE!   ");
        lcd.setCursor(0, 1); lcd.print("START LAUTERING?    ");
        lcd.setCursor(0, 2); lcd.print("OK     = YES        ");
        lcd.setCursor(0, 3); lcd.print("CANCEL = EXIT       ");

        if (btnWasClicked(18)) { 
            mashingDone = false;
            currPage = SUB_MENU_LAUTER; // Se till att denna är definierad i enum
            lcd.clear(); 
        }
        if (btnIsDown(16)) { 
            mashingDone = false;
            currPage = SUB_MENU2; 
            lcd.clear(); 
        }
    }
}

void handleManualValves() {
    // Vi tar bort currPage-checken helt för att alltid tillåta manuell styrning
    for (int i = 0; i < NUM_BUTTONS; i++) {
        static bool bState[NUM_BUTTONS];
        if (digitalRead(BUTTON_PINS[i]) == LOW && (millis() - lastDebounceTime[i]) > 200) {
            tone(BUZZER_PIN, 1500, 60); 
            bState[i] = !digitalRead(SOLENOID_PINS[i]); // Växla nuvarande status
            digitalWrite(BUTTON_LED_PINS[i], bState[i]);
            digitalWrite(SOLENOID_PINS[i], bState[i]);
            lastDebounceTime[i] = millis();
        }
    }
}

void playMashingCompleteMelody() {
    int notes[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6, NOTE_G5, NOTE_C6};
    int durations[] = {100, 100, 100, 300, 100, 500};
    lcd.clear();
    lcd.setCursor(1, 1); lcd.print("MASHING COMPLETE!");
    for (int i = 0; i < 6; i++) {
        if (i < NUM_BUTTONS) digitalWrite(BUTTON_LED_PINS[i], HIGH);
        tone(BUZZER_PIN, notes[i], durations[i]);
        delay(durations[i] + 20);
        noTone(BUZZER_PIN);
    }
    delay(4000); 
    for (int i = 0; i < NUM_BUTTONS; i++) { digitalWrite(BUTTON_LED_PINS[i], LOW); }
}

void page_Lautering() {
    static bool lauteringActive = false;
    static unsigned long lastFlowCheck = 0;
    static unsigned long lastPulseCount = 0;
    static float flowRate = 0.0;
    static float spargeLitersAtStart = 0.0; 
    
    unsigned long now = millis();
    
    // Hämta data
    noInterrupts(); 
    unsigned long pOut = pulseCountOut; 
    interrupts();
    
    float totalLitersOut = (float)pOut / calibrationFactor;

    // 1. BERÄKNA FLÖDESHASTIGHET UT
    if (now - lastFlowCheck >= 1000) {
        unsigned long pulsesSinceLast = pOut - lastPulseCount;
        flowRate = ((float)pulsesSinceLast / calibrationFactor) * 60.0;
        lastFlowCheck = now;
        lastPulseCount = pOut;
    }

    if (!lauteringActive) {
        // --- VÄNTELÄGE ---
        noInterrupts(); unsigned long pIn = pulseCountIn; interrupts();
        spargeLitersAtStart = (float)pIn / calibrationFactor;

        lcd.setCursor(0, 0); lcd.print("READY TO LAUTER?    ");
        lcd.setCursor(0, 1); lcd.print("HLT VOL: "); lcd.print(spargeLitersAtStart, 1); lcd.print("L");
        lcd.setCursor(0, 3); lcd.print("PRESS OK TO START  ");

        if (btnWasClicked(18)) {
            lauteringActive = true;
            noInterrupts(); pulseCountOut = 0; interrupts();
            lastPulseCount = 0;
            
            // Öppna ventilerna och tänd knapparna
            digitalWrite(LAUTER_PIN, HIGH);      
            digitalWrite(BUTTON_LED_PINS[4], HIGH); // Vit knapp (Lauter)
            digitalWrite(WORT_TO_BOILER, HIGH); 
            digitalWrite(BUTTON_LED_PINS[3], HIGH); // Grön knapp (Boiler)
            lcd.clear();
        }
    } else {
        // --- LAKNING PÅGÅR ---
        lcd.setCursor(0, 0); lcd.print("LAUTERING...        ");
        
        // Kolla om ventilerna är öppna (manuellt eller auto)
        if (digitalRead(LAUTER_PIN) == LOW || digitalRead(WORT_TO_BOILER) == LOW) {
            lcd.setCursor(14, 0); lcd.print("PAUSED");
        } else {
            lcd.setCursor(14, 0); lcd.print("RUN   ");
        }

        lcd.setCursor(0, 1); lcd.print("BOILER: "); 
        lcd.print(totalLitersOut, 1); lcd.print(" L");

        lcd.setCursor(0, 2); lcd.print("FLOW: "); 
        lcd.print(flowRate, 1); lcd.print(" L/m  ");
        
        if (flowRate > 1.5) { 
            lcd.setCursor(14, 2); lcd.print("FAST!");
            if (now % 1000 < 500) tone(BUZZER_PIN, 1500, 20); 
        } else {
            lcd.setCursor(15, 2); lcd.print(" "); // Rensa FAST-texten
        }

        float spargeRemaining = spargeLitersAtStart - totalLitersOut;
        if (spargeRemaining < 0) spargeRemaining = 0;
        
        lcd.setCursor(0, 3); lcd.print("HLT REM: "); 
        lcd.print(spargeRemaining, 1); lcd.print(" L  ");

        // Stopp-villkor
        if (totalLitersOut >= (batchVolInt * BOIL_OFF_PCT)) {
            stopAllOperations(); // Stänger allt och piper
            lcd.setCursor(0, 0); lcd.print("LAUTERING COMPLETE! ");
            if (btnWasClicked(18)) {
                lauteringActive = false;
                currPage = SUB_MENU2;
                lcd.clear();
            }
        }
        
        // Avbryt helt
        if (btnIsDown(16)) {
            stopAllOperations();
            lauteringActive = false;
            currPage = SUB_MENU2;
            lcd.clear();
        }
    }
}
