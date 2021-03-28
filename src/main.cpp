#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"


bool debug = 1; // 0 off | 1 on
#define MAX_SENSOR_INT 4
#define MAX_SENSOR_OUT 4
#define BUTTON_OFF 12
#define BUTTON_ON 11
#define Exhaust_tolerance 150 //
#define RANGE 1.85
byte looptime = 1; //Minuto
#define desired_difference 3
#define FILE_NAME "teste.dat"

// Sensores 01 {0x28, 0xF1, 0x73, 0xFD, 0x0A, 0x00, 0x00, 0xFE }
// Sensores 02 {0x28, 0xBD, 0xA7, 0xFC, 0x0A, 0x00, 0x00, 0x38 }
// Sensores 03 {0x28, 0xEA, 0x73, 0xFC, 0x0A, 0x00, 0x00, 0xD2 }
// Sensores 04 {0x28, 0xAC, 0x55, 0xFC, 0x0A, 0x00, 0x00, 0x71 }
// Sensores 05 {0x28, 0x0D, 0xEB, 0xFC, 0x0A, 0x00, 0x00, 0xF7 }
// Sensores 06 {0x28, 0x48, 0xCA, 0xFC, 0x0A, 0x00, 0x00, 0x5C }
// Sensores 07 {0x28, 0xCC, 0x4E, 0xFC, 0x0A, 0x00, 0x00, 0xBE }
// Sensores 08 {0x28, 0x9D, 0x59, 0xFC, 0x0A, 0x00, 0x00, 0x8A }
// Sensores 09 {0x28, 0x90, 0xE3, 0xFC, 0x0A, 0x00, 0x00, 0x32 }
// Sensores 10 {0x28, 0x3F, 0xB7, 0xFC, 0x0A, 0x00, 0x00, 0xC0 }
// Sensores 11 {0x28, 0x14, 0xAC, 0xFC, 0x0A, 0x00, 0x00, 0x82 }
// Sensores 12 {0x28, 0x14, 0x2E, 0xFD, 0x0A, 0x00, 0x00, 0x45 }
// Sensores 13 {0x28, 0x3E, 0x7C, 0xFD, 0x0A, 0x00, 0x00, 0x2A }
// Sensores 14 {0x28, 0x19, 0xD8, 0xFC, 0x0A, 0x00, 0x00, 0xBA }
// Sensores 15 {0x28, 0xBC, 0x95, 0xFC, 0x0A, 0x00, 0x00, 0x08 }
// Sensores 16 {0x28, 0x20, 0x60, 0xFC, 0x0A, 0x00, 0x00, 0xB0 }

uint8_t sensor_int[MAX_SENSOR_INT][8] = {
     {0x28, 0x9D, 0x59, 0xFC, 0x0A, 0x00, 0x00, 0x8A },
     {0x28, 0x9D, 0x59, 0xFC, 0x0A, 0x00, 0x00, 0x8A },
     {0x28, 0xBC, 0x95, 0xFC, 0x0A, 0x00, 0x00, 0x08 },
     {0x28, 0xF1, 0x73, 0xFD, 0x0A, 0x00, 0x00, 0xFE }
   };
                                      
uint8_t sensor_out[MAX_SENSOR_OUT][8] = {
    {0x28, 0xBC, 0x95, 0xFC, 0x0A, 0x00, 0x00, 0x08 },
    {0x28, 0x9D, 0x59, 0xFC, 0x0A, 0x00, 0x00, 0x8A },
    {0x28, 0xF1, 0x73, 0xFD, 0x0A, 0x00, 0x00, 0xFE },
    {0x28, 0xF1, 0x73, 0xFD, 0x0A, 0x00, 0x00, 0xFE }
  };

float temp_int[MAX_SENSOR_INT];
float temp_out[MAX_SENSOR_OUT];

float average_inside, average_outside, tempdiff, min_targetdiff, max_targetdiff;
bool FLAG_HEATING, FLAG_EXHAUST;


int __cont = 0;
float __sum = 0;

int CS_PIN = 10;

RTC_DS1307 rtc;

File file;
#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4);




byte heppy[8] = {
  0b00000,
  0b11011,
  0b11011,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};

byte neve[8] = {
  0b00000,
  0b00000,
  0b10101,
  0b01110,
  0b11011,
  0b01110,
  0b10101,
  0b00000
};

byte fire[8] = {
  0b10010,
  0b01001,
  0b01001,
  0b10010,
  0b10010,
  0b10010,
  0b01001,
  0b01001
};


byte indor[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b10101
};
byte outdor[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00100,
  0b11111
};

byte correto[8] = {
  0b00000,
  0b00001,
  0b00010,
  0b00010,
  0b10100,
  0b01000,
  0b00000,
  0b00000
};


void stater_datalooger(){
  pinMode(CS_PIN, OUTPUT);
  lcd.setCursor(3, 1);
  if (SD.begin(CS_PIN)){
    lcd.print(F("SD was started"));
  } else{
    lcd.print(F("SD was not started"));
  }
    lcd.setCursor(3, 3);
  if (!rtc.begin()){
    lcd.print(F("RTC not found!!"));
  }
  lcd.setCursor(3, 2);
  if (!rtc.isrunning())
  {
    lcd.print(F("RTC not operating!"));
  }else{
    lcd.print(F("RTC operating!"));
  }
  if(debug == 1 ){
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  delay(5000);
}

float sum_temp(float valor){
  if(valor>-50 && valor<61){
      __sum = __sum + valor;
      __cont = __cont + 1;
      return valor;
    }else{
      return -999;
    }
}


float average(){
  if(__cont >0){
    return __sum/__cont;
  }else{
    return -999;
  }
}

/**
 * Start of code block responsible for collecting the temperature
 *
 **/
void _average_temp(){
  __sum = 0;
  __cont = 0;
  for (int i = 0; i < MAX_SENSOR_OUT; i++){
    sensors.requestTemperatures();
    temp_out[i] = sum_temp(sensors.getTempC(sensor_out[i]));
    delay(50);
  }
  average_outside = average();
  __sum = 0;
  __cont = 0;
  for(int i = 0; i < MAX_SENSOR_INT; i++){
    sensors.requestTemperatures();
    temp_int[i] = sum_temp(sensors.getTempC(sensor_int[i]));
    delay(50);
  }
  average_inside = average();
  
}

void read_temp_diff() {
  _average_temp();
  tempdiff = average_inside - average_outside;
}
/**
 * End of code block responsible for collecting the temperature
 *
 **/

/**
 * Simulate the button press by means of a relay, 
 * 0 for the off button and 1 for the on button
 * turn_on_or_off
 */
void exhaust_turn_on_or_off(bool status){
  if(status == LOW){
    FLAG_EXHAUST = 0;
    digitalWrite(BUTTON_OFF, HIGH);
    delay(1000);
    digitalWrite(BUTTON_OFF, LOW);
  }else{
    FLAG_EXHAUST = 1;
    digitalWrite(BUTTON_ON, HIGH);
    delay(1000);
    digitalWrite(BUTTON_ON, LOW);
  }
}


void heating_turn_on_or_off(bool status){
  if(status == LOW){
    FLAG_HEATING = 0;
    digitalWrite(1, LOW);
  }else{
    FLAG_HEATING = 1;
    digitalWrite(1, HIGH);
  }

}


void all_off(){
  heating_turn_on_or_off(LOW);
  exhaust_turn_on_or_off(LOW);
}
void print_debug_status(){
  if(debug == 1){
    lcd.setCursor(18, 3);
    lcd.print("#");  
  }
}


void setup() {
  max_targetdiff = desired_difference + RANGE;
  min_targetdiff = desired_difference - RANGE;
  
  pinMode(BUTTON_ON, OUTPUT);
  pinMode(BUTTON_OFF, OUTPUT);
  pinMode(3, INPUT);
  pinMode(4, OUTPUT);
  pinMode(1, OUTPUT);
  
  all_off();
  
  sensors.begin();
  lcd.begin();
  lcd.backlight();
  lcd.createChar(0, heppy);
  lcd.createChar(1, neve);
  lcd.createChar(2, fire);
  lcd.createChar(6, indor);
  lcd.createChar(7, outdor);
  lcd.createChar(3, correto);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(F("System started"));
  print_debug_status();
  stater_datalooger();
  

}

void loop() {
  if(average_inside == -999 || average_outside == -999){
        
        lcd.clear();  
        lcd.setCursor(0, 0);
        lcd.print(F("     !!Error!!     "));
        lcd.setCursor(0, 2);
        lcd.print(F(" Temperature sensor "));
        lcd.setCursor(0, 3);
        lcd.print(F(" error, I can't act "));
        print_debug_status();
        all_off();  
        my_delay();
    
  }else{
    if(tempdiff >= min_targetdiff && tempdiff <= max_targetdiff){
      if (tempdiff <= (max_targetdiff-(max_targetdiff*Exhaust_tolerance/100))){
          all_off();
      }
      
    }
    else{
      climate_control_actions();  
    }
    read_temp_diff();
    record_SD();
    for(int i = 0; i < looptime*6; i++ ){
        lcd_print(1);
        read_temp_diff();
        lcd_print(0);
    }
      

    
    //my_delay();
  }

  
  
}




void climate_control_actions(){
  if (tempdiff < min_targetdiff) {
      exhaust_turn_on_or_off(LOW);    //turn air con off
      heating_turn_on_or_off(HIGH);   //turn heating on
  }
  if (tempdiff > max_targetdiff) {
      heating_turn_on_or_off(LOW);   //turn heating off
      exhaust_turn_on_or_off(HIGH); //turn air con off
   }
 }


void lcd_print(byte view) {
  lcd.clear();
  lcd.setCursor(0, 0);
  print_tm_f(min_targetdiff);
  lcd.print(F(" "));
  print_tm_f(desired_difference); 
  lcd.print(F(" "));
  print_tm_f(max_targetdiff);
  lcd.print(F(" "));
  print_tm_f(tempdiff);
  

  lcd.setCursor(0, 1);
  
  lcd.write((uint8_t)6);
  lcd.print(F(" "));
  for (int i=0; i < MAX_SENSOR_INT; i++){
    print_good_or_bad(temp_int[i]);
  }
  lcd.setCursor(10, 1);
  lcd.print(F("| "));
  print_tm_f(average_inside);
  
  lcd.setCursor(0, 2);
  lcd.write((uint8_t)7);
  lcd.print(F(" "));
  for (int i=0; i < MAX_SENSOR_OUT; i++){
      print_good_or_bad(temp_out[i]);
    }
  lcd.setCursor(10, 2);
  lcd.print(F("| "));
  print_tm_f(average_outside);
  
  
  lcd.setCursor(0, 3);
  if(view == 0){
        lcd.write((uint8_t)2);
        lcd.print(F(" "));
        if (FLAG_HEATING == HIGH ) {
          lcd.write((uint8_t)3);
        }else{
          lcd.print(F("x"));
        } 
        lcd.print(F(" "));
        lcd.write((uint8_t)1);
        lcd.print(F(" "));
        if (FLAG_EXHAUST == HIGH)  {
          lcd.write((uint8_t)3);
        }else{
          lcd.print(F("x"));
        }
        lcd.print(F("   | FQ "));
        lcd.print(looptime);
   }else{
        DateTime t = rtc.now();
        lcd.print(F("Data: "));
        lcd.print(t.year());
        lcd.print(F("-"));
        print_decimal0(t.month());
        lcd.print(F("-"));
        print_decimal0(t.day());
        print_debug_status();
        print_status();
        delay(3150);
        lcd.setCursor(0, 3);
        lcd.print(F("Hora: "));
        print_decimal0(t.hour());
        lcd.print(F(":"));
        print_decimal0(t.minute());
        lcd.print(F(":"));
        print_decimal0(t.second());
        lcd.print(F("  "));
        print_debug_status();
        print_status();
        
      }

  print_debug_status();
  print_status();
  delay(3150);
}


void print_tm_f(float temp){
  if (temp >= 0) {
    lcd.print("+");
  }
  lcd.print(temp,1);
  
}

void print_good_or_bad(float d){
  if (d != -999)  {
    lcd.write((uint8_t)3);
  }else{
    lcd.print(F("x"));
  }
}



void my_delay(){
    if(debug == 0){
      delay((looptime*60000)-15000);  
    }else{
      delay(looptime*5000);
     }
}




void print_status(){
   lcd.setCursor(19, 3);
   if(tempdiff >= min_targetdiff && tempdiff <= max_targetdiff){
    lcd.write((uint8_t)0);
  }else{
    if(tempdiff < min_targetdiff ){
      lcd.write((uint8_t)1);
    }else{
      lcd.write((uint8_t)2);
    }
  }
}


int open_file_recording(char filename[]){
  file = SD.open(filename, FILE_WRITE);
  if (file){
    return 1;
  }else{
    return 0;
  }
}
 
void fecha_arquivo(){
  if(file){
    file.close();
  }
}


//Grava dados no cartao SD
void record_SD(){
  DateTime t = rtc.now();
  if(!SD.exists(FILE_NAME)){
    open_file_recording(FILE_NAME);
    file.print(F("date,int_temp_1,int_temp_2,int_temp_3,int_temp_4,out_temp_1,out_temp_2,out_temp_3,out_temp_4,heating,exhaust"));
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print(F(" creating file "));
    lcd.setCursor(0, 3);
    lcd.print(FILE_NAME);
    delay(5000);
    file.close();
    
  }
  bool status_file = open_file_recording(FILE_NAME);
  if(debug==1){
    if(status_file){
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print(F("Data successfully"));
      lcd.setCursor(1, 2);
      lcd.print(F("recorded on"));
      lcd.setCursor(1, 3);
      lcd.print(FILE_NAME);
      delay(2000);
    }else{
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print(F("ERROR when writing"));
      lcd.setCursor(1, 2);
      lcd.print(F("data to"));
      lcd.setCursor(1, 3);
      lcd.print(FILE_NAME);
      delay(2000);
    }
  }
  file.print(t.year());
  
  file.print("-");
  decimal_number_writing(t.month());
  file.print("-");
  decimal_number_writing(t.day());
  file.print(" ");
  decimal_number_writing(t.hour());
  file.print(F(":"));
  decimal_number_writing(t.minute());
  file.print(F(":"));
  decimal_number_writing(t.second());
  file.print(F(","));
  for (int i = 0; i < 4;i++){
    if(i<MAX_SENSOR_INT){
      file.print(temp_int[i]);
      file.print(F(","));
    }else{
      file.print(F("NA"));
      file.print(F(","));
    }
  }
  for (int i = 0; i < 4;i++){
    if(i<MAX_SENSOR_OUT){
      file.print(temp_out[i]);
      file.print(F(","));
    }else{
      file.print(F("NA"));
      file.print(F(","));
    }
  }
  file.print(F(","));
  file.print(FLAG_EXHAUST);
  file.print(FLAG_HEATING);

  

  
  fecha_arquivo();
}

void decimal_number_writing(int valor){
  if (valor < 10)
  {
    file.print(F("0"));
  }
  file.print(valor);
}

void print_decimal0(int _time ){
   if (_time < 10){
      lcd.print(F("0"));
  }
  lcd.print(_time);
}
 

 
