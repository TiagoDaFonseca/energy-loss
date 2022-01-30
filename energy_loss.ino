/******************Energy Loss version 4.0 ***********************
 *
 * Developper: Tiago Cunha (PhD research Fellow, CEFITEC, 2017)
 *
 * Obs: The following is the code required to run on arduino to
 * enable control of the hemispherical analyser located in the
* crossed molecular beam setup. It can be used with a labview
* interface or just turn the Serial monitor on from the IDE.
 *
 * Functions:
* - Energy loss
* - Voltage calibration as a function of the channel range
*
*
 *****************************************************************/
#define Power 256 // necessary for low pass filter running average
#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h> #include <stdlib.h>

// Edit the acquisition parameters here:
int timeBin = 1000; // in millisecond
int initial_ch = 0;// Initial channel
int final_ch = 510; // Final channel
float calFactor = 200.0/4.84; // Voltage divisor calibration factor int cycles = 1;
//Panel LED’s
int ledVermelho = 5;
int ledVerde = 6;
//MCP41010 digipot
int CS = 10; // assigned to pin 10
//Voltage tuning parameter
int vout = 3;
//relay 4
int relay = 8; // assigned to pin 8
//LCD
LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3,POSITIVE);
//Other Global variables:
volatile int signalIn = 2; // assigned to pin 2 String settings;
float voltage = 0;
int alpha = 15; // changeable from 0-255


//Functions:
// Pulse Counter
// >>> it measures the signal from the channeltron detector
int pulseCounter (int timeBin){
  unsigned long initial_time = 0;
  int count = 0;
  initial_time = millis(); //defines initial time.
  while(millis() - initial_time < timeBin){ // limit the time window
    if(digitalRead(signalIn) == LOW){
      if(digitalRead(signalIn) == HIGH){
        count++; // It increments every time it detects a pulse
      }
    } 
  }
  return count;
}
// digiPotWrite function
// It transfers data to MCP41010 digital pot trough SPI communication //Notes:
// - 0x80: mid point
// - 0x00: highest resistance
// - 0xFF: lowest resistance
void digiPotWrite (byte value){ 
  digitalWrite(CS,LOW); 
  SPI.transfer(B00010001);// in binary notation 
  SPI.transfer(value);
  digitalWrite(CS,HIGH);
}
//Update Settings
void updateSettings(){
  String instringT, instringC , instringI , instringF;
  int i, n;
  do{
    if(settings[i]==’C’){
      for(n=i+1;settings[n]!=’T’;n++){
        if(isDigit(settings[n])) instringC += (char)settings[n];
        i=n; 
      }
      cycles = instringC.toInt();
    }
    if(settings[i]==’T’){
      for(n=i+1;settings[n]!=’I’;n++){
        if(isDigit(settings[n])) instringT += (char)settings[n];
        i=n;
      }
      timeBin = instringT.toInt();
    }
    if(settings[i]==’I’){
      for(n=i+1;settings[n]!=’F’;n++){
        if(isDigit(settings[n])) instringI += (char)settings[n];
        i=n;
      }
      initial_ch = instringI.toInt(); 
    }
    if(settings[i]==’F’){
      for(n=i+1;settings[n]!=’\0’;n++){
        if(isDigit(settings[n])) instringF += (char)settings[n];
        i=n; 
      }
      final_ch = instringF.toInt(); 
    }
    i++; 
    }while(i<settings.length());
}

void Measuring (int *flag){ 
  lcd.setCursor(0,1); 
  lcd.print("Measuring");
  if (*flag == 0){ 
    lcd.print("."); 
    lcd.print(" "); 
    *flag = 1;
  }
  else if(*flag == 1){
    lcd.print(".."); 
    lcd.print(" "); 
    *flag = 2;
  }
  else if(*flag == 2){
    lcd.print("..."); 
    lcd.print(" "); 
    *flag = 3;
  }
  else{
    lcd.print(" "); 
    *flag = 0;
  } 
}

// Proress bar widths
byte p1[8]={0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
byte p2[8]={0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18};
byte p3[8]={0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C};
byte p4[8]={0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E};
byte p5[8]={0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};


//Initialization: runs once
void setup() {
  pinMode(signalIn,INPUT);
  pinMode(CS, OUTPUT);
  pinMode(ledVermelho,OUTPUT);
  pinMode(ledVerde,OUTPUT);
  pinMode(relay,OUTPUT);
  pinMode(vout,OUTPUT);
  //characters creation 
  lcd.createChar(0, p1); lcd.createChar(1, p2); lcd.createChar(2, p3); lcd.createChar(3, p4); lcd.createChar(4, p5);
  Serial.begin(9600); SPI.begin(); lcd.begin(16,2);
  digitalWrite(ledVerde,HIGH);
  digitalWrite(ledVermelho,HIGH);
  //initialize LCD
  lcd.setCursor(0,0); lcd.print("Alkali ELoss v4"); delay(200);
}
//Working routine: runs repeatedly
void loop() {
  int counts;
  String cmd;
  int outputLen = 0;
  char Buffer[30];
  int v = 0;
  float FiltValue = 0;
  //Scanning variables
  int level, initial_level, final_level; int flag = -1;
  //Starting
  delay(1000);
  FiltValue=0;
  digitalWrite(ledVermelho,LOW); 
  lcd.setCursor(0,0); 
  lcd.print("Alkali ELoss v4"); 
  lcd.print(" ");
  
  while(Serial.available() == 0){
    //rotina que defina o digipot no maximo
    //e que leia a tensao e contagens no lcd.Standby mode 
    digitalWrite(relay,LOW);
    delay(100);
    digiPotWrite(255);
    //Acquire voltage
    v = analogRead(A0);
    voltage = v*(5.0/1024)*calFactor;
    //E.M.A. algorithm 
    if(FiltValue == 0){
      FiltValue = voltage;
    }
  else{
    FiltValue = (alpha*voltage + (Power - alpha)*FiltValue)/Power; }
    //Output average value 
    Serial.println(FiltValue);
    //visualize voltage in LCD 
    lcd.setCursor(0,1); 
    lcd.print("V = "); 
    lcd.print(FiltValue); 
    lcd.print("V"); 
    lcd.print(" "); 
    delay(100);
  }
  cmd = Serial.readString();
  switch(cmd[0]){
    case ’l’: //energy loss 
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Energy Loss");
      digitalWrite(ledVermelho,HIGH);
      digitalWrite(ledVerde,LOW);
      initial_level = initial_ch - 255; final_level = final_ch - 255;
      
      //digitalWrite(relay,HIGH);
      for(int cycle = 0;cycle<cycles; cycle++){
        for(level=initial_level;level<=final_level;level++){
          //activates the relay and switches from +5V to -5V the voltahe into digipot 
          if(level==0){
          digitalWrite(relay,HIGH);
          }
          int Byte = abs(level);
          // Sets digipot position
          digiPotWrite(Byte);
          delay(500);
          //Acquires signal during timeBin
          counts = pulseCounter(timeBin);
          //Outputs the signal trough Serial port
          outputLen = sprintf(Buffer,"%dF%d\n",level+255,counts);
          for(int k=0; k<outputLen;k++){
            Serial.print(Buffer[k]); 
          }
          //Measuring status
          Measuring(&flag);
          //reset the relay to LOW state after 1 sweep
          digitalWrite(relay, LOW);
          //Zero the counter signalIn = 0; 
          if(Serial.available()>0){
            if(Serial.read() == ’s’) goto Exit; 
          }
        } 
      }
      goto Exit;
    break;
  case ’v’: // calibration
    lcd.clear(); lcd.setCursor(0,0); lcd.print("Volt calibration");
    digitalWrite(ledVerde,LOW);
    digitalWrite(ledVermelho,HIGH);
    initial_level = initial_ch - 255; final_level = final_ch - 255;
    for(level=initial_level;level<=final_level;level++){
      //Activates relay to switch voltage polarity
      if(level == 0){
      digitalWrite(relay,HIGH);
      }
      // Sets digipot position
      digiPotWrite(abs(level));
      delay(1000);
      //Acquire voltage
      v = analogRead(A0);
      voltage = v*(5.0/1024)*calFactor;
      //Outputs the signal trough Serial port
      char buff[10];
      char sentence[50];
      //E.M.A. algorithm 
      if(FiltValue == 0){
        FiltValue = voltage;
      }
      else{
        FiltValue = (alpha*voltage + (Power - alpha)*FiltValue)/Power;
      }
      dtostrf(FiltValue, 4, 3, buff);
      outputLen = sprintf(Buffer,"%dF",level+255);
      outputLen = sprintf(sentence,"%s%s\n",Buffer, buff);
      for(int k=0; k<outputLen;k++)
      { 
        Serial.print(sentence[k]);
      }
      //Measuring status
      Measuring(&flag);
      if(Serial.available()>0){ 
        if(Serial.read() == ’s’) goto Exit;
      } 
    }
    digitalWrite(relay,LOW);
    goto Exit;
    break;
  case ’o’: // settings ( ’options’)
    settings = cmd; // To avoid changing cmd string
    updateSettings(); // routine to update settings
    break;
  default:
    Exit: // stand by conditions
    digitalWrite(ledVerde,HIGH);
    digitalWrite(ledVermelho,LOW);
    digitalWrite(relay, LOW);
  } 
}
