#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

#include "Leeds_Help_Arduino.cpp"


bool debug = 0; // 0 off | 1 on
bool status_file;
#define MAX_SENSOR_INT 4
#define MAX_SENSOR_OUT 4
#define looptime 1 //Minuto
#define FILE_NAME "b.dat"


uint8_t sensor_int[MAX_SENSOR_INT][8] = {
     TEMPERATURE_SENSOR_09,
     TEMPERATURE_SENSOR_10,
     TEMPERATURE_SENSOR_11,
     TEMPERATURE_SENSOR_12
   };
                                      
uint8_t sensor_out[MAX_SENSOR_OUT][8] = {
    TEMPERATURE_SENSOR_13,
    TEMPERATURE_SENSOR_14,
    TEMPERATURE_SENSOR_15,
    TEMPERATURE_SENSOR_16
  };

#define number_Moving_average 8
float temp_int_media[MAX_SENSOR_INT][number_Moving_average];
float temp_out_media[MAX_SENSOR_OUT][number_Moving_average];
float temp_int[MAX_SENSOR_INT];
float temp_out[MAX_SENSOR_OUT];
float average_inside, average_outside, tempdiff;

bool FLAG_EXHAUST; // Flags 


int __cont = 0;
float __sum = 0;

int CS_PIN = 10;

RTC_DS1307 rtc;

File file;
#define ONE_WIRE_BUS 3
#define BUTTON_OFF 4
#define BUTTON_ON 1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4);


void (*funcReset)() = 0;
int erros = 0;


void stater_datalooger(){
  pinMode(CS_PIN, OUTPUT);
  lcd.setCursor(3, 1);
  if (SD.begin(CS_PIN)){
    lcd.print(F("SD ok"));
  } else{
    lcd.print(F("SD fall"));
  }
    lcd.setCursor(3, 3);
  if (!rtc.begin()){
    lcd.print(F("RTC fall"));
  }
  lcd.setCursor(3, 2);
  if (!rtc.isrunning())
  {
    lcd.print(F("RTC fall"));
  }else{
    lcd.print(F("RTC ok"));
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


float average(int int_or_out){
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


void start_sensor(){
  for (int i = 0; i < MAX_SENSOR_OUT; i++){
    for (int j = 0; j < number_Moving_average; j++){
      temp_out_media[i][j] = -999;
    }
  }
  for (int i = 0; i < MAX_SENSOR_INT; i++){
    for (int j = 0; j < number_Moving_average; j++){
      temp_int_media[i][j] = -999;
    }
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



void print_debug_status(){
  if(debug == 1){
    lcd.setCursor(18, 3);
    lcd.print("#");  
  }
}


void setup() {
  start_sensor();
  pinMode(BUTTON_ON, OUTPUT);
  pinMode(BUTTON_OFF, OUTPUT);
  pinMode(3, INPUT);
 
  
  sensors.begin();
  lcd.begin();
  lcd.backlight();
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
/**
 * End Prints
 */

void climate_control_actions(){
  DateTime t = rtc.now();

  if (t.hour()*60+t.minute() >= 8*60 && t.hour()*60+t.minute() <= 16*60+30 ) {
    FLAG_EXHAUST = 1;
    digitalWrite(BUTTON_ON, HIGH);
    delay(2000);
    digitalWrite(BUTTON_ON, LOW);
  }else{
    FLAG_EXHAUST = 0;
    digitalWrite(BUTTON_OFF, HIGH);
    delay(2000);
    digitalWrite(BUTTON_OFF, LOW);
  }
  
}


void lcd_print(byte view) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("DIFF: "));
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
  DateTime t = rtc.now();
  
  lcd.setCursor(0, 3);
  if(view == 0){
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
        lcd.print(F("Data: "));
        lcd.print(t.year());
        lcd.print(F("-"));
        print_decimal0(t.month());
        lcd.print(F("-"));
        print_decimal0(t.day());
        print_debug_status();
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
        
      }

  print_debug_status();
  delay(3000);
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
    status_file = open_file_recording();
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
      delay(6000);
    }
    file.print(F("date,int_temp_1,int_temp_2,int_temp_3,int_temp_4,out_temp_1,out_temp_2,out_temp_3,out_temp_4,exhaust,v\n"));
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print(F(" creating file! "));
    lcd.setCursor(0, 3);
    lcd.print(FILE_NAME);
    delay(5000);
    file.close();
    
  }
  status_file = open_file_recording();
  if(status_file){
      if(debug==1){
        lcd.clear();
        lcd.setCursor(2, 1);
        lcd.print(F("Data successfully"));
        lcd.setCursor(1, 2);
        lcd.print(F("recorded on"));
        lcd.setCursor(1, 3);
        lcd.print(FILE_NAME);
        delay(2000);
      }
    }else{
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print(F("ERROR when writing"));
      lcd.setCursor(1, 2);
      lcd.print(F("data to"));
      lcd.setCursor(1, 3);
      lcd.print(FILE_NAME);
      delay(5000);
      funcReset();
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
  file.print(F("1.0.0"));
  file.print(F("\n"));
  fecha_arquivo();
}


 
void loop() {
  read_temp_diff();
  lcd.clear(); 
  climate_control_actions();  
  if(average_inside == -999 || average_outside == -999){
        lcd.clear();  
        lcd.setCursor(0, 0);
        lcd.print(F("     !!Error!!     "));
        lcd.setCursor(0, 2);
        lcd.print(F(" Temperature sensor "));
        lcd.setCursor(0, 3);
        lcd.print(F(" Error: "));
        lcd.print(erros);
        print_debug_status();
        delay(5000);
        erros = erros + 1;
        if(erros > 20){
          funcReset();
        }


  }else{
    if(erros > 0){
      erros = erros - 1;
    }
    record_SD();
    for(int i = 0; i < looptime*6; i++ ){
        lcd_print(1);
        read_temp_diff();
        climate_control_actions();
        lcd_print(0);
    }

  }
  
}