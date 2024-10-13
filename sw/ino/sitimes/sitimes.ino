#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define encoder0PinA 2
#define encoder0PinB 3


volatile int encoder0Pos1 = 0;
volatile int encoder0Pos;


char freq[32];
char part[64];
char serial[32];
char fabDate[32];

LiquidCrystal_I2C lcd(0x27,20,4);

int clkPin = A5;
int dataPin = A4;
enum buttonAction{NONE,PRESS, LONG_PRESS};

typedef enum dataType {UINT32, FLOAT, STRZ};

enum menuState {INFO, EDIT1, EDIT2, EDIT3, EDIT4, SETPULL};

struct registerInfo{
  uint8_t address;
  char description[16];
  uint8_t type;
  float scale;
  char unit[6];
};

uint8_t displayLines[4] = {4, 17, 18, 19};

#define NUM_REG 17
struct registerInfo registerMap[NUM_REG] {
  {0x50, "", STRZ,1,"" }, //part number
  {0x52, "Freq: ", STRZ, 1, "" }, //Frequency
  {0x56, "Lot/Ser: ", STRZ, 1, "" }, //lot and serial
  {0x57, "Man: ", STRZ, 1, "" }, //date
  {0x61, "Pull: ", FLOAT, 1e9, "ppb" },
  {0x62, "Range: ", FLOAT, 1e9, "ppb" },
  {0x63, "Age comp: ", FLOAT, 1e9, "ppb/s" },
  {0x64, "Max ramp: ", FLOAT, 1e9, "ppb" },
  {0xA0, "Uptime: ", UINT32, 1, "s" },
  {0xA1, "Temp:", FLOAT, 1, "C" },
  {0xA3, "Voltage: ", FLOAT, 1000, "mV" },
  {0xA7, "HTR PWR: ", FLOAT, 1000, "mW" },
  {0xAB, "Tot offs: ", FLOAT, 1e9, "ppm" },
  {0xAE, "Errorflag: ", UINT32, 1, "" },
  {0xAF, "Stable: ", UINT32, 1, "" },
  {0xB0, "Temp err: ", FLOAT, 1, "°C" },
  {0xB1, "HTR TGT: ", FLOAT, 1000, "mW" },
};


enum buttonAction getButtonAction(){
  static uint8_t pressCount = 0;
  static uint8_t lastButtonState = 1;
  uint8_t buttonState = digitalRead(8);
  enum buttonAction ba = NONE;

  if(buttonState == lastButtonState and buttonState == 0){
    Serial.print("pressCount ");Serial.println(pressCount);
    pressCount += 1;
  }

  if(buttonState != lastButtonState and buttonState != 0){
    if(pressCount < 2){ ba = PRESS; Serial.println("PRESS");}
    if(pressCount > 10){ ba = LONG_PRESS; Serial.println("LONG_PRESS");}
  }

  if(buttonState != lastButtonState) pressCount = 0;
  lastButtonState = buttonState;
  return ba;
}



void doEncoderA(){

  // look for a low-to-high on channel A
  if (digitalRead(encoder0PinA) == HIGH) { 

    // check channel B to see which way encoder is turning
    if (digitalRead(encoder0PinB) == LOW) {  
      encoder0Pos1 = encoder0Pos1 + 1;         // CW
    } 
    else {
      encoder0Pos1 = encoder0Pos1 - 1;         // CCW
    }
  }

  else   // must be a high-to-low edge on channel A                                       
  { 
    // check channel B to see which way encoder is turning  
    if (digitalRead(encoder0PinB) == HIGH) {   
      encoder0Pos1 = encoder0Pos1 + 1;          // CW
    } 
    else {
      encoder0Pos1 = encoder0Pos1 - 1;          // CCW
    }
  }
  //Serial.println (encoder0Pos, DEC);          
  // use for debugging - remember to comment out
encoder0Pos = encoder0Pos1/2;
}

void doEncoderB(){

  // look for a low-to-high on channel B
  if (digitalRead(encoder0PinB) == HIGH) {   

    // check channel A to see which way encoder is turning
    if (digitalRead(encoder0PinA) == HIGH) {  
      encoder0Pos1 = encoder0Pos1 + 1;         // CW
    } 
    else {
      encoder0Pos1 = encoder0Pos1 - 1;         // CCW
    }
  }

  // Look for a high-to-low on channel B

  else { 
    // check channel B to see which way encoder is turning  
    if (digitalRead(encoder0PinA) == LOW) {   
      encoder0Pos1 = encoder0Pos1 + 1;          // CW
    } 
    else {
      encoder0Pos1 = encoder0Pos1 - 1;          // CCW
    }
  }
  encoder0Pos = encoder0Pos1/2;
} 



void readRegData(uint8_t regAddr, char *dst, uint16_t nb){
  Wire.beginTransmission(0x63);
  Wire.write(regAddr);
  Wire.endTransmission();

  Wire.requestFrom(0x63, nb);
  Wire.readBytes(dst, nb);
}

void writeRegData(uint8_t regAddr, char *src, uint16_t nb){
  Wire.beginTransmission(0x63);
  Wire.write(regAddr);
  Wire.write(src, nb);
  Wire.endTransmission();
}

void setup() {
  pinMode(8, INPUT_PULLUP);
  //digitalWrite(8, HIGH);
  pinMode(encoder0PinA, INPUT_PULLUP); 
  pinMode(encoder0PinB, INPUT_PULLUP); 
  //digitalWrite(encoder0PinA, HIGH);
  //digitalWrite(encoder0PinB, HIGH);


  // encoder pin on interrupt 0 (pin 2)
  attachInterrupt(0, doEncoderA, CHANGE);

  // encoder pin on interrupt 1 (pin 3)
  attachInterrupt(1, doEncoderB, CHANGE);  


  Wire.begin();
Wire.setClock(400000L);
  readRegData(0x50, part, 64);
  readRegData(0x52, freq, 32);
  readRegData(0x56, serial, 32);
  readRegData(0x57, fabDate, 32);

  
  Serial.begin(115200);
  float pull = 33.95e-9;
  writeRegData(0x61, (char*) &pull, 4);

  float age = 1.652099089848935e-14; //4.527264263705885e-14*0+0*1.4372145662104234e-14;
  writeRegData(0x63, (char*) &age, 4);
  //lcd.init();                    
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print(part);
  lcd.setCursor(0,1); lcd.print(freq);
  lcd.setCursor(0,2); lcd.print(serial);
  lcd.setCursor(0,3); lcd.print(fabDate);
  delay(200);
  lcd.clear();
}

void displayRegister(uint8_t regidx){
  uint32_t rdi;
  float rdf;
  char rdstr[32];
  float scale = registerMap[regidx].scale;
  //Serial.print(F("Displaying register: ")); Serial.println(regidx);
  //Serial.println(registerMap[regidx].description);
  lcd.print(registerMap[regidx].description);
  switch(registerMap[regidx].type){
    case UINT32:
      readRegData(registerMap[regidx].address, (char*) &(rdi),4);
      lcd.print(rdi);
      //else lcd.print(scale*((float)rdi));
    break;
    case FLOAT:
      readRegData(registerMap[regidx].address, (char*) &(rdf), 4);
      lcd.print(scale*rdf);
    break;
    case STRZ:
      readRegData(registerMap[regidx].address, (char*) &(rdstr), 32);
      rdstr[20]=0;
      lcd.print(rdstr);
    break;
  }
  lcd.print(registerMap[regidx].unit);
}

#define NUM_DISPLAY_ITEM (NUM_REG+3)
void displayItem(uint8_t i){
  float resTemp, tempError, htrPower, htrTarget, voltage;
  uint32_t stab, uptime;
  if(i < NUM_REG) displayRegister(i);
  else if(i < NUM_DISPLAY_ITEM){
    switch(i){
      case NUM_REG:
      readRegData(0xA1, (char*)&resTemp, 4);
      readRegData(0xB0, (char*)&tempError, 4);
        lcd.print("Temp: "); lcd.print(resTemp); lcd.print("/");lcd.print(resTemp+tempError);
        break;
      case NUM_REG+1:
      readRegData(0xA7, (char*)&htrPower, 4);
      readRegData(0xB1, (char*)&htrTarget, 4);
        lcd.print("HTR: "); lcd.print(1000*htrPower); lcd.print("/");lcd.print(1000*htrTarget);
        break;
      case NUM_REG+2:
      readRegData(0xA3, (char*)&voltage, 4);
      readRegData(0xAF, (char*)&stab, 4);
      readRegData(0xA0, (char*)&uptime, 4);
        lcd.print(voltage); lcd.print("V s: "); lcd.print(stab);lcd.print(" "); lcd.print(((float)uptime)/3600);lcd.print("h");
        break;
    }
  }
}

void displayMenuItems(){
  
      lcd.setCursor(0,0); displayItem(displayLines[0]);
      lcd.setCursor(0,1); displayItem(displayLines[1]);
      lcd.setCursor(0,2); displayItem(displayLines[2]);
      lcd.setCursor(0,3); displayItem(displayLines[3]);
}

void menu(){
  static menuState ms = INFO;
  static int refPos = 0;
  static float refPull;
  float pull;
  buttonAction ba = getButtonAction();
  Serial.print(ba); Serial.print(" "); Serial.println(ms);
  Serial.print("INFO"); Serial.println(INFO);
  Serial.print("EDIT1"); Serial.println(EDIT1);
  switch(ms){
    case INFO:
      Serial.println("INFO");
      displayMenuItems();
      encoder0Pos = displayLines[0];
      if(ba == PRESS){encoder0Pos = displayLines[0]; ms = EDIT1;}
      if(ba == LONG_PRESS){readRegData(0x61,(char*) &refPull, 4);lcd.clear(); refPos = encoder0Pos; ms = SETPULL;}
      
      break;
    case EDIT1:
      displayMenuItems();
      displayLines[0] = encoder0Pos;
      displayLines[0] %= NUM_DISPLAY_ITEM; 
      lcd.setCursor(0,0);lcd.print(F("                    ")); 
      if(ba == PRESS){encoder0Pos = displayLines[1]; ms = EDIT2;}
      if(ba == LONG_PRESS) ms = INFO;
      break;

    case EDIT2:
      displayMenuItems();
      displayLines[1] = encoder0Pos;
      displayLines[1] %= NUM_DISPLAY_ITEM; 
      lcd.setCursor(0,1);lcd.print(F("                    ")); 
      if(ba == PRESS){encoder0Pos = displayLines[2]; ms = EDIT3;}
      if(ba == LONG_PRESS) ms = INFO;
      break;
      
    case EDIT3:
      displayMenuItems();
      displayLines[2] = encoder0Pos;
      displayLines[2] %= NUM_DISPLAY_ITEM; 
      lcd.setCursor(0,2);lcd.print(F("                    ")); 
      if(ba == PRESS){ encoder0Pos = displayLines[3];ms = EDIT4;}
      if(ba == LONG_PRESS) ms = INFO;
      break;

    case EDIT4:
      displayMenuItems();
      displayLines[3] = encoder0Pos;
      displayLines[3] %= NUM_DISPLAY_ITEM; 
      lcd.setCursor(0,3);lcd.print(F("                    ")); 
      if(ba == PRESS) ms = INFO;
      if(ba == LONG_PRESS) ms = INFO;
      break;
    case SETPULL:
      lcd.setCursor(0,0);
      displayRegister(4);
      lcd.setCursor(0,0);
      displayRegister(4);
      lcd.setCursor(0,0);
      displayRegister(4);
      lcd.setCursor(0,0);
      displayRegister(4);
      pull = refPull+(encoder0Pos-refPos)*0.5e-10;
      Serial.print("Pull");
      Serial.println(1e9*pull);
      writeRegData(0x61, (char*) &pull, 4);
      if(ba == PRESS) ms = INFO;
    //default:
    //  ms = INFO;
  }
}

void loop() {
  char tmpStr[256];
  char partStr[256];
  uint32_t intnum;
  float flnum;
  float resTemp;
  float targetTemp;
  float tempError;
  float htrPower;
  float htrTarget;
  float voltage;
  float freqPull;
  uint32_t stab;
  uint32_t uptime;
  
  /*readRegData(0x50, partStr, 32);
  Serial.print(F("Part: ")); Serial.println(partStr);

  readRegData(0x52, tmpStr, 32);
  Serial.print(F("Frequency: ")); Serial.println(tmpStr);

  readRegData(0x56, tmpStr, 32);
  Serial.print(F("Serial: ")); Serial.println(tmpStr);

  readRegData(0x57, tmpStr, 32);
  Serial.print(F("Fab date: ")); Serial.println(tmpStr);

  readRegData(0xA0, (char*)&uptime, 4);
  Serial.print(F("Uptime: ")); Serial.println(uptime);

  readRegData(0xA1, (char*)&resTemp, 4);
  Serial.print(F("Resoator temp: ")); Serial.println(resTemp);

  readRegData(0xA3, (char*)&voltage, 4);
  Serial.print(F("UC voltage: ")); Serial.println(voltage);

  readRegData(0xA7, (char*)&htrPower, 4);
  Serial.print(F("Heater power: ")); Serial.println(htrPower);

  readRegData(0xAB, (char*)&flnum, 4);
  Serial.print(F("Written offset: ")); Serial.println(1e9*flnum);

  readRegData(0xAE, (char*)&intnum, 4);
  Serial.print(F("Error status: ")); Serial.println(intnum);

  readRegData(0xAF, (char*)&stab, 4);
  Serial.print(F("Stability status: ")); Serial.println(stab);

  readRegData(0xB0, (char*)&tempError, 4);
  Serial.print(F("Temperature error: ")); Serial.println(tempError);

  readRegData(0xB1, (char*)&htrTarget, 4);
  Serial.print(F("Power target: ")); Serial.println(htrTarget);

  readRegData(0x62, (char*)&flnum, 4);
  Serial.print(F("Pull range ")); Serial.println(1e9*flnum);
  
  readRegData(0x61, (char*)&freqPull, 4);
  Serial.print(F("Pull value ")); Serial.println(1e9*freqPull);*/
  
  //lcd.setCursor(0,0); lcd.print("Temp: "); lcd.print(resTemp); lcd.print("/");lcd.print(resTemp+tempError);
  //lcd.setCursor(0,1); lcd.print("HTR: "); lcd.print(1000*htrPower); lcd.print("/");lcd.print(1000*htrTarget);
  //lcd.setCursor(0,2); lcd.print(voltage); lcd.print("V s: "); lcd.print(stab);lcd.print(" "); lcd.print(((float)uptime)/3600);lcd.print("h");
  //lcd.setCursor(0,3); lcd.print("Pull: ");lcd.print(1e9*freqPull);lcd.print("ppb");

  menu();

  delay(10);
}
