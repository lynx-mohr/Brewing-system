
// ******************************************************************
// ||                            ROOT MENU                         ||
// ******************************************************************

void page_RootMenu(){

//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1;

//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
  updateDisplay = false;

lcd.clear();
lcd.setCursor(0,0); printSelected(1, sub_Pos); lcd.print("Manual operation");
lcd.setCursor(0,1); printSelected(2, sub_Pos); lcd.print("Automated brew"); 

}

if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}

//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == ROOT_MENU_CNT){sub_Pos = 1;} else{sub_Pos++;}

updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = ROOT_MENU_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU1; return;
  case 2: currPage = SUB_MENU2; return;

}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};
}
}


// ******************************************************************
// ||                            SUB MENU1 (manual operations)     ||
// ******************************************************************

void page_SubMenu1() {

//tracks when entered att top of loop. 
uint32_t loopStartMs;

  sensors.requestTemperatures(); 
  TEMP_HEATER = sensors.getTempCByIndex(1);
  TEMP_MASH = sensors.getTempCByIndex(2);

  bool updateDisplay = true;
  unsigned long lastUpdate = 0;

  unsigned long lastPulseCount = 0;
  float liters = 0;
  float flowRate = 0;

  while (true) {
    handleButtonPress();  // uppdaterar buttonState[] + solenoiderna

    unsigned long now = millis();

    // räkna om flöde 1 ggr/sek
    if (now - lastUpdate >= 1000) {
      lastUpdate = now;

      noInterrupts();
      unsigned long pulses = pulseCount;
      interrupts();

      unsigned long diff = pulses - lastPulseCount;
      lastPulseCount = pulses;

      liters = (float)pulses / calibrationFactor;
      flowRate = ((float)diff / calibrationFactor) * 60.0f;

      updateDisplay = true;
    }

    if (updateDisplay) {
      updateDisplay = false;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Manual operation");

      lcd.setCursor(0,1);
      lcd.print("Heater: ");
      lcd.print(TEMP_HEATER,1);
      lcd.print("C");

      lcd.setCursor(0,2);
      lcd.print("Flow:");
      lcd.print(flowRate,1);
      lcd.print(" L/m ");

      lcd.setCursor(0,3);
      lcd.print(liters,1);
      lcd.print(" L total");

      // Exempel: skriv status för solenoider via serial (kan läggas på LCD om du vill)
      for (int i = 0; i < NUM_BUTTONS; i++) {
        Serial.print("Solenoid ");
        Serial.print(i+1);
        Serial.print(": ");
        Serial.println(buttonState[i] ? "OPEN" : "CLOSED");
      }
    }
//keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

  }
}



// ******************************************************************
// ||                            SUB MENU2 (input headers)         ||
// ******************************************************************

int page_SubMenu2(){

//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1;

//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;

while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
  updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); printSelected(1, sub_Pos); lcd.print("Mash thickness");
lcd.setCursor(0,1); printSelected(2, sub_Pos); lcd.print("Mash temperature"); 
lcd.setCursor(0,2); printSelected(3, sub_Pos); lcd.print("Grain Weight"); 
lcd.setCursor(0,3); printSelected(4, sub_Pos); lcd.print("Run Program"); 
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}

//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU2_CNT){sub_Pos = 1;} else{sub_Pos++;}

updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU2_CNT;} else{sub_Pos--;}

updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = ROOT_MENU; return;}

//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU3; return;
  case 2: currPage = SUB_MENU5; return;
  case 3: currPage = SUB_MENU7; return;
  case 4: currPage = SUB_MENU10; return; 

}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}


}
// ******************************************************************
// ||                            SUB MENU3 (Mash thickness)        ||
// ******************************************************************

void page_SubMenu3(){
//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Mash thickness"); 
lcd.setCursor(0,2); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print(",- liter/kg");
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU3_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU3_CNT; numberCount = SUB_MENU2_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = SUB_MENU1; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     mashThicknessInt = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU4; return;
  case 2: currPage = SUB_MENU4; return;
  case 3: currPage = SUB_MENU4; return;
  case 4: currPage = SUB_MENU4; return;
  case 5: currPage = SUB_MENU4; return;
  case 6: currPage = SUB_MENU4; return;
  case 7: currPage = SUB_MENU4; return;
  case 8: currPage = SUB_MENU4; return;
  case 9: currPage = SUB_MENU4; return;
  case 10: currPage = SUB_MENU4; return;
  case 11: currPage = SUB_MENU4; return;
  case 12: currPage = SUB_MENU4; return;
  case 13: currPage = SUB_MENU4; return;
  case 14: currPage = SUB_MENU4; return;
  case 15: currPage = SUB_MENU4; return;
  case 16: currPage = SUB_MENU4; return;
  case 17: currPage = SUB_MENU4; return;
  case 18: currPage = SUB_MENU4; return;
  case 19: currPage = SUB_MENU4; return;
  case 20: currPage = SUB_MENU4; return;
  

}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}

// ******************************************************************
// ||                            SUB MENU4      (Mash Thickness)   ||
// ******************************************************************

void page_SubMenu4(){
//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Mash thickness"); 
lcd.setCursor(0,2); lcd.print(mashThicknessInt); lcd.print(","); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print(" liter/kg"); 
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU4_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU4_CNT; numberCount = SUB_MENU2_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = ROOT_MENU; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     mashThicknessDec = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU2; return;
  case 2: currPage = SUB_MENU2; return;
  case 3: currPage = SUB_MENU2; return;
  case 4: currPage = SUB_MENU2; return;
  case 5: currPage = SUB_MENU2; return;
  case 6: currPage = SUB_MENU2; return;
  case 7: currPage = SUB_MENU2; return;
  case 8: currPage = SUB_MENU2; return;
  case 9: currPage = SUB_MENU2; return;
  case 10: currPage = SUB_MENU2; return;


}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}

// ******************************************************************
// ||                            SUB MENU5   (Mash Temp)           ||
// ******************************************************************

void page_SubMenu5(){
//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {62, 63, 64, 65, 66, 67, 68, 69, 70, 71}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Mash temperature"); 
lcd.setCursor(0,2); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print(",- degrees");
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU5_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU5_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = SUB_MENU1; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     mashTempInt = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU6; return;
  case 2: currPage = SUB_MENU6; return;
  case 3: currPage = SUB_MENU6; return;
  case 4: currPage = SUB_MENU6; return;
  case 5: currPage = SUB_MENU6; return;
  case 6: currPage = SUB_MENU6; return;
  case 7: currPage = SUB_MENU6; return;
  case 8: currPage = SUB_MENU6; return;
  case 9: currPage = SUB_MENU6; return;
  case 10: currPage = SUB_MENU6; return;
   

}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}

// ******************************************************************
// ||                            SUB MENU6     (Mash temp)         ||
// ******************************************************************

void page_SubMenu6(){
//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Mash temperature"); 
lcd.setCursor(0,2); lcd.print(mashTempInt); lcd.print(","); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print(" degrees"); 
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU6_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU6_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = ROOT_MENU; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     mashTempDec = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU2; return;
  case 2: currPage = SUB_MENU2; return;
  case 3: currPage = SUB_MENU2; return;
  case 4: currPage = SUB_MENU2; return;
  case 5: currPage = SUB_MENU2; return;
  case 6: currPage = SUB_MENU2; return;
  case 7: currPage = SUB_MENU2; return;
  case 8: currPage = SUB_MENU2; return;
  case 9: currPage = SUB_MENU2; return;
  case 10: currPage = SUB_MENU2; return;


}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}


// ******************************************************************
// ||                            SUB MENU7      Grain weight       ||
// ******************************************************************

void page_SubMenu7(){
//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Grain weight"); 
lcd.setCursor(0,2); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print(",-- kg");
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU3_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU3_CNT; numberCount = SUB_MENU2_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = SUB_MENU1; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     grainWeightInt = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU8; return;
  case 2: currPage = SUB_MENU8; return;
  case 3: currPage = SUB_MENU8; return;
  case 4: currPage = SUB_MENU8; return;
  case 5: currPage = SUB_MENU8; return;
  case 6: currPage = SUB_MENU8; return;
  case 7: currPage = SUB_MENU8; return;
  case 8: currPage = SUB_MENU8; return;
  case 9: currPage = SUB_MENU8; return;
  case 10: currPage = SUB_MENU8; return;
  case 11: currPage = SUB_MENU8; return;
  case 12: currPage = SUB_MENU8; return;
  case 13: currPage = SUB_MENU8; return;
  case 14: currPage = SUB_MENU8; return;
  case 15: currPage = SUB_MENU8; return;
  case 16: currPage = SUB_MENU8; return;
  case 17: currPage = SUB_MENU8; return;
  case 18: currPage = SUB_MENU8; return;
  case 19: currPage = SUB_MENU8; return;
  case 20: currPage = SUB_MENU8; return;
  

}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}
// ******************************************************************
// ||                            SUB MENU8                         ||
// ******************************************************************

void page_SubMenu8(){
//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Grain weight"); 
lcd.setCursor(0,2); lcd.print(grainWeightInt); lcd.print(","); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print("- kg"); 
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU6_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU6_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = ROOT_MENU; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     grainWeightDec1 = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU9; return;
  case 2: currPage = SUB_MENU9; return;
  case 3: currPage = SUB_MENU9; return;
  case 4: currPage = SUB_MENU9; return;
  case 5: currPage = SUB_MENU9; return;
  case 6: currPage = SUB_MENU9; return;
  case 7: currPage = SUB_MENU9; return;
  case 8: currPage = SUB_MENU9; return;
  case 9: currPage = SUB_MENU9; return;
  case 10: currPage = SUB_MENU9; return;


}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}

// ******************************************************************
// ||                            SUB MENU9                         ||
// ******************************************************************

void page_SubMenu9(){

//tracks when entered att top of loop. 
uint32_t loopStartMs;

//flag for update display
uint8_t updateDisplay = true;

//selected item pointer
uint8_t sub_Pos = 1; 

//Array to hold all the numbers from 1 to 20
int wholeNumbers[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 

int numberCount = 1;


//tracks button states
boolean btn_Up_WasDown = false;
boolean btn_Down_WasDown = false; 
boolean btn_Accept_WasDown = false;
boolean btn_Cancel_WasDown = false;


while(true){

 loopStartMs = millis(); 

//print display;
if(updateDisplay == true){
  
updateDisplay = false;
lcd.clear();
lcd.setCursor(0,0); lcd.print("Grain weight"); 
lcd.setCursor(0,2); lcd.print(grainWeightInt); lcd.print(","); lcd.print(grainWeightDec1); lcd.print(wholeNumbers[sub_Pos-1]); lcd.print(" kg"); 
}

//Capture the button down states
if(btnIsDown(BTN_UP)){btn_Up_WasDown = true;}
if(btnIsDown(BTN_DOWN)){btn_Down_WasDown = true;}
if(btnIsDown(BTN_ACCEPT)){btn_Accept_WasDown = true;}
if(btnIsDown(BTN_CANCEL)){btn_Cancel_WasDown = true;}


//move the pointer down
if(btn_Down_WasDown && btnIsUp(BTN_DOWN)){

if(sub_Pos == SUB_MENU6_CNT){sub_Pos = 1; numberCount = 1;} else{sub_Pos++;}


updateDisplay = true;

btn_Down_WasDown = false;

}

//move the pointer up
if(btn_Up_WasDown && btnIsUp(BTN_UP)){

if(sub_Pos == 1){sub_Pos = SUB_MENU6_CNT;} else{sub_Pos--;}


updateDisplay = true;

btn_Up_WasDown = false;

}

//cancel to last menu (in this case: root menu)
if(btn_Cancel_WasDown && btnIsUp(BTN_CANCEL)){ currPage = ROOT_MENU; return;}


//move to selected page
if(btn_Accept_WasDown && btnIsUp(BTN_ACCEPT)){

     grainWeightDec2 = wholeNumbers[sub_Pos-1];

switch (sub_Pos){
  
  case 1: currPage = SUB_MENU2; return;
  case 2: currPage = SUB_MENU2; return;
  case 3: currPage = SUB_MENU2; return;
  case 4: currPage = SUB_MENU2; return;
  case 5: currPage = SUB_MENU2; return;
  case 6: currPage = SUB_MENU2; return;
  case 7: currPage = SUB_MENU2; return;
  case 8: currPage = SUB_MENU2; return;
  case 9: currPage = SUB_MENU2; return;
  case 10: currPage = SUB_MENU2; return;


}
}
 //keep a specific pace
 while(millis() - loopStartMs < 25) {delay(2);};

}
}

// ******************************************************************
// ||                            SUB MENU10   -Run program         ||
// ******************************************************************


void page_SubMenu10_enter() {
  // 1) Räkna fram slutvärden från användarens inmatning
  mashThicknessFinal = mashThicknessInt + 0.1 * mashThicknessDec;
  mashTempFinal      = mashTempInt      + 0.1 * mashTempDec;
  grainWeightFinal   = grainWeightInt   + grainWeightDec1 * 0.1 + grainWeightDec2 * 0.01; 

  sensors.requestTemperatures();
 // grainTemp = sensors.getTempCByIndex(1); Tillfällig: 
 grainTemp = 20;
  strikeTempFunction(mashThicknessFinal, mashTempFinal, grainTemp);

  // 2) Nollställ states
  sm10_showingPreset  = true;
  sm10_startPresetMs  = millis();
  sm10_lastUpdateMs   = 0;
  sm10_lastPulseCount = 0;


    targetVolume = mashWaterVolFunction(grainWeightFinal, mashThicknessFinal);

  // 3) Första preset-vyn
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Preset Values:");
  lcd.setCursor(0,1); lcd.print("Rate: ");      lcd.print(mashThicknessFinal,1); lcd.print(" l/kg   ");
  lcd.setCursor(0,2); lcd.print("Mash temp: "); lcd.print(mashTempFinal,1);      lcd.print(" C      ");
  lcd.setCursor(0,3); lcd.print("Grain: ");     lcd.print(grainWeightFinal,2);   lcd.print(" kg     ");

  // Vi startar INTE fyllning här än; vi väntar tills preset-skärmen försvinner
  filling = false;

    // Start filling the heater with water  
  liters = 0;
  pulseCount = 0;
  filling = true;  

  sm10_inited = true;


}

void page_SubMenu10_loop() {
  unsigned long now = millis();

  // Efter 4 sekunder, gå från preset-vy till "kör"-vy
  if (sm10_showingPreset && now - sm10_startPresetMs >= 4000UL) {
    sm10_showingPreset = false;
    lcd.clear();
  }

  // Uppdatera fyllnings-info tills volymen är nådd
  if (!sm10_showingPreset && !volumeReached && now - sm10_lastUpdateMs >= 1000UL) {
    sm10_lastUpdateMs = now;

    // Läs pulser säkert
    noInterrupts();
    unsigned long pulses = pulseCount;
    interrupts();

    liters = (float)pulses / calibrationFactor;
    unsigned long diff = pulses - sm10_lastPulseCount;
    flowRate = ((float)diff / calibrationFactor) * 60.0f;
    sm10_lastPulseCount = pulses;

    lcd.setCursor(0,0);
    lcd.print("Mash target: ");
    lcd.print(targetVolume, 2);
    lcd.print("L   ");

    lcd.setCursor(0,1);
    lcd.print("Water: ");
    lcd.print(liters,2);
    lcd.print("L   ");

    lcd.setCursor(0,2);
    lcd.print("Flow:  ");
    lcd.print(flowRate,2);
    lcd.print("L/min   ");

    lcd.setCursor(0,3);
    lcd.print("Filling   ");

    // Ventilstyrning
    if (filling && liters >= targetVolume) {
      digitalWrite(SOLENOID_PINS[0], LOW);
      filling = false;
      volumeReached = true;
      Serial.println("Målvolym uppnådd. Ventilen stängd.");
    }
    if (filling) {
      digitalWrite(SOLENOID_PINS[0], HIGH);
    }
  }

  // När volymen är nådd → byt vy EN gång
  if (volumeReached && !switchedToHeatingView) {
    lcd.clear();
    switchedToHeatingView = true;
  }

  // --- Värmestyrning ---
  if (volumeReached) {
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temp, 1);
    lcd.print(" C     ");

    lcd.setCursor(0, 1);
    lcd.print("Strike: ");
    lcd.print(strikeTemp, 1);
    lcd.print(" C     ");

    if (heatingOn) {
      lcd.setCursor(0, 2);
      lcd.print("Heating ON     ");
    } else {
      lcd.setCursor(0, 2);
      lcd.print("Heating OFF    ");
    }

    lcd.setCursor(0, 3);
    lcd.print("Mashing imminent  ");

    // Relästyrning med hysteresis
    if (!heatingOn && temp < strikeTemp - 1) {
      digitalWrite(RELAY_PIN, HIGH);
      heatingOn = true;
    } 
    else if (heatingOn && temp > strikeTemp + 1) {
      digitalWrite(RELAY_PIN, LOW);
      heatingOn = false;
    }
  }
}
  


/*
void page_SubMenu10() {


    // --- Calculate mash values ---
    mashThicknessFinal = mashThicknessInt + 0.1*mashThicknessDec;
    mashTempFinal = mashTempInt + 0.1*mashTempDec;
    grainWeightFinal = grainWeightInt + grainWeightDec1*0.1 + grainWeightDec2*0.01;


    // --- Timing variables ---
    unsigned long startPresetMillis = millis();
    unsigned long lastFlowUpdate = 0;

    bool showingPreset = true;

    unsigned long lastPulseCount = 0;


    // --- Main loop ---
    while(true) {
    
        unsigned long currentMillis = millis();

        // 1️⃣ Show preset values for 4 seconds
        if(showingPreset) {
            lcd.clear();
            lcd.setCursor(0,0); lcd.print("Preset Values:");
            lcd.setCursor(0,1); lcd.print("Rate: "); lcd.print(mashThicknessFinal,1); lcd.print(" l/kg");
            lcd.setCursor(0,2); lcd.print("Mash temp: "); lcd.print(mashTempFinal,1); lcd.print(" C");
            lcd.setCursor(0,3); lcd.print("Grain: "); lcd.print(grainWeightFinal,2); lcd.print(" kg");

            showingPreset = false;            // only display once
            startPresetMillis = currentMillis; // mark start time
        }

        // 2️⃣ After 4 seconds, switch to mash water + flow display
        if(!showingPreset && currentMillis - startPresetMillis >= 4000) {
            // Update flow and volume every 1 second
            if(currentMillis - lastFlowUpdate >= 1000) {
                lastFlowUpdate = currentMillis;

                // Copy pulseCount safely
                noInterrupts();
                unsigned long pulses = pulseCount;
                interrupts();

                // Total liters
                liters = (float)pulses / calibrationFactor;

                // Flow rate in L/min
                unsigned long pulseDiff = pulses - lastPulseCount;
                flowRate = ((float)pulseDiff / calibrationFactor) * 60.0; // L/min
                lastPulseCount = pulses;

                // --- Display mash water and flow ---
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Mash water: ");
                lcd.print(mashWaterVolFunction(grainWeightFinal, mashThicknessFinal), 2);
                lcd.print(" l");

                lcd.setCursor(0,1);
                lcd.print("Water: ");
                lcd.print(liters,2);
                lcd.print(" L");

                lcd.setCursor(0,2);
                lcd.print("Flow: ");
                lcd.print(flowRate,2);
                lcd.print(" L/min");

                lcd.setCursor(0,3);
                lcd.print("SOLENOID ON");
            }
        }

        // Keep solenoid active
        digitalWrite(SOLENOID_PINS[0], HIGH);
    }
}
*/



// ******************************************************************
// ||                            TOOLS - DISPLAY                   ||
// ******************************************************************

void printSelected(uint8_t p1, uint8_t p2){

if(p1 == p2){ lcd.print("->");}

else{lcd.print("  ");}

}

// ******************************************************************
// ||                            TOOLS - BUTTON PRESSING           ||
// ******************************************************************

bool btnIsDown(int btn){
return digitalRead(btn) == LOW && digitalRead(btn) == LOW;

}

bool btnIsUp(int btn){
return digitalRead(btn) == HIGH && digitalRead(btn) == HIGH;

} 

//Plays a short tone when button is pressed
void playTone(){

    tone(BUZZER_PIN, 3000);
  delay(100);
  noTone(BUZZER_PIN);
}