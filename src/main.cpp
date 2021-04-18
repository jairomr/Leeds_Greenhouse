#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

#include "Leeds_Help_Arduino.cpp"


bool debug = 0; // 0 off | 1 on
#define MAX_SENSOR_INT 4
#define MAX_SENSOR_OUT 4
#define Exhaust_tolerance 70 // 0 a 100
#define RANGE 1.85
#define looptime 1 //Minuto
#define desired_difference 0
#define FILE_NAME "teste.dat"


uint8_t sensor_int[MAX_SENSOR_INT][8] = {
     TEMPERATURE_SENSOR_01,
     TEMPERATURE_SENSOR_02,
     TEMPERATURE_SENSOR_03,
     TEMPERATURE_SENSOR_04
   };
                                      
uint8_t sensor_out[MAX_SENSOR_OUT][8] = {
    TEMPERATURE_SENSOR_05,
    TEMPERATURE_SENSOR_06,
    TEMPERATURE_SENSOR_07,
    TEMPERATURE_SENSOR_08
  };

#define number_Moving_average 15
float temp_int_media[MAX_SENSOR_INT][number_Moving_average];
float temp_out_media[MAX_SENSOR_OUT][number_Moving_average];
float temp_int[MAX_SENSOR_INT];
float temp_out[MAX_SENSOR_OUT];
float average_inside, average_outside, tempdiff, min_targetdiff, max_targetdiff; 


bool FLAG_HEATING, FLAG_EXHAUST; // Flags 


int __cont = 0;
float __sum = 0;

int CS_PIN = 10;

RTC_DS1307 rtc;

File file;
#define ONE_WIRE_BUS 3
#define BUTTON_OFF 12
#define BUTTON_ON 11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4);


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


float temperature_validation(float valor){
  if(valor>-50 && valor<61){
      return valor;
    }else{
      return -999;
    }
}


float average(bool int_or_out){
  if(int_or_out == 0 ){
    __sum = 0;
    __cont = 0;
    for (int i = 0; i < MAX_SENSOR_OUT; i++){
      if(temp_out[i] != -999){
        __sum = __sum + temp_out[i];
        __cont = __cont + 1;
      }
      
    }
    if(__cont > 0){
      return __sum/__cont;
    }
    return -999;
  }else{
    __sum = 0;
    __cont = 0;
    for (int i = 0; i < MAX_SENSOR_INT; i++){
      if(temp_int[i] != -999){
        __sum = __sum + temp_int[i];
        __cont = __cont + 1;
      }
      
    }
    if(__cont > 0){
      return __sum/__cont;
    }
    return -999;

  }
}

/**
 * Start of code block responsible for collecting the temperature
 *
 **/
void _average_temp(){
  //externo
  for (int i = 0; i < MAX_SENSOR_OUT; i++){
    sensors.requestTemperatures();
    for (int j = number_Moving_average - 1; j > 0; j--){
      temp_out_media[i][j] = temp_out_media[i][j-1];
    }
    temp_out_media[i][0] = temperature_validation(sensors.getTempC(sensor_out[i]));
    __sum = 0;
    __cont = 0;
    for (int j = 0; j < number_Moving_average; j++){
      if(temp_out_media[i][j] != -999){
        __sum = __sum + temp_out_media[i][j];
        __cont = __cont + 1;
      }
    }
    if(__cont > 0){
      temp_out[i] = __sum/__cont;
    }else{
      temp_out[i] = -999;
    }
    delay(50);
  }
  average_outside = average(0);

  // interno
  for (int i = 0; i < MAX_SENSOR_INT; i++){
    sensors.requestTemperatures();
    for (int j = number_Moving_average - 1; j > 0; j--){
      temp_int_media[i][j] = temp_int_media[i][j-1];
    }
    temp_int_media[i][0] = temperature_validation(sensors.getTempC(sensor_int[i]));
    __sum = 0;
    __cont = 0;
    for (int j = 0; j < number_Moving_average; j++){
      if(temp_int_media[i][j] != -999){
        __sum = __sum + temp_int_media[i][j];
        __cont = __cont + 1;
      }
    }
    if(__cont > 0){
      temp_int[i] = __sum/__cont;
    }else{
      temp_int[i] = -999;
    }
    delay(50);
  }
  average_inside = average(1);
  
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



/**
 * Prints
 */
void decimal_number_writing(int valor){
  if (valor < 10){
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
/**
 * End Prints
 */



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


int open_file_recording(){
  file = SD.open(FILE_NAME, FILE_WRITE);
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
    open_file_recording();
    file.print(F("date,int_temp_1,int_temp_2,int_temp_3,int_temp_4,out_temp_1,out_temp_2,out_temp_3,out_temp_4,heating,exhaust\n"));
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print(F(" creating file "));
    lcd.setCursor(0, 3);
    lcd.print(FILE_NAME);
    delay(5000);
    file.close();
    
  }
  bool status_file = open_file_recording();
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
  file.print(FLAG_EXHAUST);
  file.print(F(","));
  file.print(FLAG_HEATING);
  file.print(F("\n"));
  fecha_arquivo();
}


 
void loop() {
  read_temp_diff();
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
        delay(10000);
    
  }else{
    if(tempdiff >= min_targetdiff && tempdiff <= max_targetdiff){
      if (tempdiff <= (max_targetdiff-((RANGE+RANGE)*Exhaust_tolerance/100))){
          all_off();
      }
    }
    else{
      climate_control_actions();  
    }
    record_SD();
    for(int i = 0; i < looptime*6; i++ ){
        lcd_print(1);
        read_temp_diff();
        lcd_print(0);
    }

  }
  
}