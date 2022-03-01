#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

#include "Leeds_Help_Arduino.cpp"

#define ESTUFA_A // Definacao para qual estufa ira ser compilado o codigo
#define DEBUG_OFF // ON or OFF

#define start_hour 8
#define start_minutes 0

#define end_hour 16
#define end_minutes 30


bool status_file;


#define looptime 1 //Minuto

float int_1,int_2,int_3,int_4,out_1,out_2,out_3,out_4

float average_inside, average_outside, tempdiff;

bool FLAG_EXHAUST; // Flags 


byte __cont__ = 0;
float __sum__ = 0;

int CS_PIN = 10;

RTC_DS1307 rtc;

File file;
#define ONE_WIRE_BUS 3
#define BUTTON_OFF 4
#define BUTTON_ON 1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4);

#ifdef ESTUFA_A
#define FILE_NAME "a.dat"
#else
#define FILE_NAME "b.dat"
#endif

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
  #ifdef DEBUG_ON
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  #endif
  delay(5000);
}


float temperature_validation(float valor){
  if(valor>-50 && valor<61){
      return valor;
    }else{
      return -999;
    }
}

void get_temp(){
  sensors.requestTemperatures();
//  delay(700);
  #ifdef ESTUFA_A
    int_1 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_01));
    //delay(700);
    int_2 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_02));
    //delay(700);
    int_3 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_03));
    //delay(700);
    int_4 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_04));
    //delay(700);
    out_1 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_05));
    //delay(700);
    out_2 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_06));
    //delay(700);
    out_3 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_07));
    //delay(700);
    out_4 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_08));
  #else
    int_1 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_09));
    //delay(700);
    int_2 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_10));
    //delay(700);
    int_3 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_11));
    //delay(700);
    int_4 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_12));
    //delay(700);
    out_1 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_13));
    //delay(700);
    out_2 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_14));
    //delay(700);
    out_3 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_15));
    //delay(700);
    out_4 = temperature_validation(sensors.getTempC(TEMPERATURE_SENSOR_16));
  #endif


}

/**
 * Start of code block responsible for collecting the temperature
 *
 **/
void average_temp(){
  __sum__ = 0
  __cont__ = 0
  if(int_1 > -999){
    __sum__ = __sum__ + int_1;
    __cont__ = __cont__ + 1
  }
  if(int_2 > -999){
    __sum__ = __sum__ + int_2;
    __cont__ = __cont__ + 1
  }
  if(int_3 > -999){
    __sum__ = __sum__ + int_3;
    __cont__ = __cont__ + 1
  }
  if(int_4 > -999){
    __sum__ = __sum__ + int_4;
    __cont__ = __cont__ + 1
  }

  average_inside = -999
  if (__cont__> 0 ){
    average_inside = __sum__/__cont__;
  }
  
  ___sum__ = 0;
  __cont__ = 0;
  if(out_1 > -999){
    __sum__ = __sum__ + out_1;
    __cont__ = __cont__ + 1
  }
  if(out_2 > -999){
    __sum__ = __sum__ + out_2;
    __cont__ = __cont__ + 1
  }
  if(out_3 > -999){
    __sum__ = __sum__ + out_3;
    __cont__ = __cont__ + 1
  }
  if(out_4 > -999){
    __sum__ = __sum__ + out_4;
    __cont__ = __cont__ + 1
  }
  average_outside = -999
  if (__cont__> 0 ){
    average_outside = __sum__/__cont__;
  }  // interno
  

}

void read_temp_diff() {
  get_temp();
  average_temp();
  tempdiff = -999
  if(average_inside > -999 & average_outside > -999){
    tempdiff = average_inside - average_outside;
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
  #ifdef DEBUG_ON
    lcd.setCursor(18, 3);
    lcd.print("#");
  #endif
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
  int clock_now = t.hour()*60+t.minute();
  if (
    clock_now >= start_hour*60+start_minutes
   && clock_now <= end_hour*60+end_minutes 
  ) {
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
  
  print_good_or_bad(int_1);
  print_good_or_bad(int_2);
  print_good_or_bad(int_3);
  print_good_or_bad(int_4);
  
  lcd.setCursor(10, 1);
  lcd.print(F("| "));
  print_tm_f(average_inside);
  
  lcd.setCursor(0, 2);
  lcd.write((uint8_t)7);
  lcd.print(F(" "));
  
  print_good_or_bad(out_1);
  print_good_or_bad(out_2);
  print_good_or_bad(out_3);
  print_good_or_bad(out_4);

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
     if(erros == 0){
        lcd.print(F("Data: "));
        lcd.print(t.year());
        lcd.print(F("-"));
        print_decimal0(t.month());
        lcd.print(F("-"));
        print_decimal0(t.day());
     }else{
        lcd.print(F("Err:"));
        lcd.print(erros);
        lcd.print(t.year());
        lcd.print(F("-"));
        print_decimal0(t.month());
        lcd.print(F("-"));
        print_decimal0(t.day());
      }
        
        #ifdef DEBUG_ON
          lcd.setCursor(18, 3);
          lcd.print("#");
        #endif
        delay(3150);
        lcd.setCursor(0, 3);
        lcd.print(F("Hora: "));
        print_decimal0(t.hour());
        lcd.print(F(":"));
        print_decimal0(t.minute());
        lcd.print(F(":"));
        print_decimal0(t.second());
        lcd.print(F("  "));
        #ifdef DEBUG_ON
          lcd.setCursor(18, 3);
          lcd.print("#");
        #endif
        
      }

  #ifdef DEBUG_ON
    lcd.setCursor(18, 3);
    lcd.print("#");
  #endif
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

void print_temperature(float valor){
  if(i<valor){
      file.print(temp_int[i]);
      file.print(F(","));
  }else{
      file.print(F("NA"));
      file.print(F(","));
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
    #ifdef DEBUG_ON
        lcd.clear();
        lcd.setCursor(2, 1);
        lcd.print(F("Data successfully"));
        lcd.setCursor(1, 2);
        lcd.print(F("recorded on"));
        lcd.setCursor(1, 3);
        lcd.print(FILE_NAME);
        delay(2000);
    #else
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print(F("ERROR when writing"));
      lcd.setCursor(1, 2);
      lcd.print(F("data to"));
      lcd.setCursor(1, 3);
      lcd.print(FILE_NAME);
      delay(5000);
      funcReset();
   #endif
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

  print_temperature(int_1);
  print_temperature(int_2);
  print_temperature(int_3);
  print_temperature(int_4);

  print_temperature(out_1);
  print_temperature(out_2);
  print_temperature(out_3);
  print_temperature(out_4);


  file.print(FLAG_EXHAUST);
  file.print(F(","));
  file.print(F("2.0.0"));
  file.print(F("\n"));
  fecha_arquivo();
}


 
void loop() {
  read_temp_diff();
  lcd.clear(); 
  climate_control_actions();  
  if(average_inside == -999 || average_outside == -999){
      erros = erros + 1;
        if(erros > 20){
          funcReset();
        }
  }else{
      if(erros > 0){
      erros = erros - 1;
      }
  }
  record_SD();
  for(int i = 0; i < looptime*6; i++ ){
    lcd_print(1);
    read_temp_diff();
    climate_control_actions();
    lcd_print(0);
  }
}