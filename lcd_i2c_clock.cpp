#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <EEPROM.h> 
#include <NeoPixelBus.h>
#include <ESP8266HTTPClient.h>
#include <string.h>

#define led_in 2
#define bat_in A0
#define lcd_p1 5 
#define lcd_p2 4  
#define zummer 13   
#define t_sensor 14 
#define led_strip 12 

#define eeprom_size 512
#define eeprom_summer_time 58 
#define ssid_adr 256
#define pass_adr 380
#define lan_serv_adr 480

const uint16_t PixelCount = 20; 
const uint8_t PixelPin = led_strip; 

LiquidCrystal_I2C lcd(0x27,lcd_p1,lcd_p2);  
OneWire oneWire(t_sensor);
DallasTemperature sensors(&oneWire);
DeviceAddress *sensorsUnique;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ua.pool.ntp.org", 3600*2, 60000);
NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);
WiFiClient client; 

int countSensors;
unsigned long timer0_interval = 1000;
unsigned long timer0;
bool summer_time,debug = false;
int seconds,minutes,hours,days,months,years,weakday,battery_int = 0;
float temperature = 0;
String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
bool time_updated = false;
float battery_voltage = 0;
String ssid_name,ssid_pass = "";
String  lan_server_name = "";
unsigned long starting_unix_sytem_time = 0;
unsigned long set_unix_system_time = 0;

 

void set_value_to_eeprom(int address,int data) {
  EEPROM.begin(eeprom_size);
  EEPROM.put(address,data); 
  EEPROM.commit();
  delay(10);
  EEPROM.end();  
  if (debug==true) {Serial.println("Time attribute "+String(address)+" saved to eeprom!");}
}

int read_value_from_eeprom(int address) {
  int data = 0;
  EEPROM.begin(eeprom_size);
  EEPROM.get(address,data);
  EEPROM.end(); 
  return(data);
  delay(10);
  if (debug==true) {Serial.println("Time attribute "+String(address)+" read from eeprom!");} 
}



String unixTimeToHumanReadable(long int secondsff) //getted https://www.geeksforgeeks.org/convert-unix-timestamp-to-dd-mm-yyyy-hhmmss-format/
{
    String ans = "";
    int daysOfMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    long int currYear, daysTillNow, extraTime, extraDays, index, date, month, hoursf, minutesf, secondsf, flag = 0;
    daysTillNow = secondsff / (24 * 60 * 60);
    extraTime = secondsff % (24 * 60 * 60);
    currYear = 1970;
    // Calculating current year and months
    while (true) {
        if (currYear % 400 == 0
            || (currYear % 4 == 0 && currYear % 100 != 0)) {
            if (daysTillNow < 366) {
                break;
            }
            daysTillNow -= 366;
        }
        else {
            if (daysTillNow < 365) {
                break;
            }
            daysTillNow -= 365;
        }
        currYear += 1;
    }
    extraDays = daysTillNow + 1;
    if (currYear % 400 == 0 || (currYear % 4 == 0 && currYear % 100 != 0)) flag = 1;
    month = 0, index = 0;
    if (flag == 1) {
        while (true) {
            if (index == 1) {
                if (extraDays - 29 < 0)
                    break;
                month += 1;
                extraDays -= 29;
            }
            else {
                if (extraDays - daysOfMonth[index] < 0) {
                    break;
                }
                month += 1;
                extraDays -= daysOfMonth[index];
            }
            index += 1;
        }
    }
    else {
        while (true) {
            if (extraDays - daysOfMonth[index] < 0) {
                break;
            }
            month += 1;
            extraDays -= daysOfMonth[index];
            index += 1;
        }
    }
    if (extraDays > 0) {
        month += 1;
        date = extraDays;
    }
    else {
        if (month == 2 && flag == 1)
            date = 29;
        else {
            date = daysOfMonth[month - 1];
        }
    }
    int wday = ((daysTillNow % 7)-1);
    // Calculating HH:MM:YYYY
    hoursf = extraTime / 3600;
    minutesf = (extraTime % 3600) / 60;
    secondsf = (extraTime % 3600) % 60;
    // Return the time
    hours=hoursf;
    minutes=minutesf;
    seconds=secondsf;
    years=currYear;  
    months=month;
    days=date;
    weakday=wday;
    ans = String(currYear)+"-"+String(month)+"-"+String(date)+"/"+String(hours)+":"+String(minutes)+":"+String(seconds)+"-"+String(wday); 
    return ans;
}


//відправка листа
String send_and_get_mesage(String mes, String url_key){
 HTTPClient http;
 bool started = false;
 String payload = "";
 int httpResponseCode = -100;
 String serverPath = "";
 serverPath = url_key+mes;
 Serial.println(serverPath);
 started = http.begin(client,serverPath);  //
 Serial.println(started);
  int br = 500;
  Serial.println("Waiting response");
  while (httpResponseCode<0){
    //httpResponseCode = http.POST(serverPath);
    httpResponseCode = http.GET();
    Serial.print(".");
    delay(1);
    br--;
    if (br<0) break;
  }
  Serial.println("end");
 delay(1);
 if (httpResponseCode>0) {
   Serial.print("HTTP Response code: ");
   Serial.println(httpResponseCode);
   payload = http.getString();
   Serial.println(payload);}
 else {
  Serial.print("Error code: ");
  Serial.println(httpResponseCode);}
 http.end(); 
 delay(900);
 return  payload;   
 }

//відлік часу
void timecalculate(){ 
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);  
  days = ptm->tm_mday;
  months = ptm->tm_mon+1;
  years = ptm->tm_year+1900;
  seconds = timeClient.getSeconds();
  minutes = timeClient.getMinutes();
  hours = timeClient.getHours();
  weakday = timeClient.getDay();   
} 

void timecalculate_lan(){
  if (set_unix_system_time==0){set_unix_system_time=send_and_get_mesage("/?get=unix_time",lan_server_name).toInt();}  //
  if (set_unix_system_time>0 and starting_unix_sytem_time==0){starting_unix_sytem_time = millis()/1000;}
  unsigned long res = (set_unix_system_time+(millis()/1000))-starting_unix_sytem_time;
  String tts=unixTimeToHumanReadable(res); 
  if (set_unix_system_time>0 and time_updated==false){ time_updated=true;}
  Serial.println(String(res)); 
  //Serial.println(tts);
  Serial.println(String(years)+"-"+String(months)+"-"+String(days)+"/"+String(hours)+":"+String(minutes)+":"+String(seconds)+"-"+String(weakday)); 
}

void go_time() { 
  if (lan_server_name!=""){
    timecalculate_lan();
    if ((minutes==59) and (seconds>59-hours) and (set_unix_system_time>0)) {tone(zummer,1000,500);}
    if ((minutes==0) and (seconds==0) and (set_unix_system_time>0)) {tone(zummer,2000,700);}
  } else {
    timecalculate();
    if ((minutes==59) and (seconds>59-hours)) {tone(zummer,1000,500);}
    if ((minutes==0) and (seconds==0)) {tone(zummer,2000,700);}    
    }
}


void write_string_EEPROM (int Addr, String Str) {
  byte lng=Str.length();
  EEPROM.begin (eeprom_size);
  if (lng>128 )  lng=128;
  EEPROM.write(Addr , lng); 
  unsigned char* buf = new unsigned char[lng+2];
  Str.getBytes(buf, lng + 1);
  Addr++;
  for(byte i = 0; i < lng; i++) {EEPROM.write(Addr+i, buf[i]); delay(10);}
  EEPROM.end();
}

char *read_string_EEPROM (int Addr) {
  EEPROM.begin(eeprom_size);
  byte lng = EEPROM.read(Addr);
  char* buf = new char[lng+2];
  Addr++;
  for(byte i = 0; i < lng; i++) buf[i] = char(EEPROM.read(i+Addr));
  buf[lng] = '\x0';
  EEPROM.end();
  return buf;
}

void readssid() {
  ssid_name=read_string_EEPROM(ssid_adr);
  ssid_pass=read_string_EEPROM(pass_adr);    
  }

void writessid() {   
  write_string_EEPROM(ssid_adr,ssid_name); 
  write_string_EEPROM(pass_adr,ssid_pass);         
  }

void serial_comms(){  
  if ((Serial.available() > 0)) {
    String ts ="";
    ts=Serial.readString();
    Serial.println("rpt>"+ts);
    if (ts=="/summer_time=1") {summer_time=true; set_value_to_eeprom(eeprom_summer_time,summer_time); Serial.println("com>summer_time=true");}
    if (ts=="/summer_time=0") {summer_time=false; set_value_to_eeprom(eeprom_summer_time,summer_time); Serial.println("com>summer_time=false");}  
    if (ts=="/debug=1") {summer_time=true;  Serial.println("com>debug=true");}
    if (ts=="/debug=0") {summer_time=false;  Serial.println("com>debug=false");}      
    if (ts=="/reset") {ESP.restart();}
    if (ts.substring(0,11)=="/ssid_name=") {ssid_name=ts.substring(11,27); writessid(); readssid(); Serial.println("Set ssid_name:"+ssid_name);} 
    if (ts.substring(0,11)=="/ssid_pass=") {ssid_pass=ts.substring(11,27); writessid(); readssid(); Serial.println("Set ssid_pass:"+ssid_pass);}
    if (ts.substring(0,12)=="/lan_server=") {lan_server_name=ts.substring(12,32); write_string_EEPROM (lan_serv_adr,lan_server_name); lan_server_name=read_string_EEPROM(lan_serv_adr); Serial.println("Set lan_server:"+lan_server_name);}
    if (ts=="/help") {Serial.println("Commands for user settings> /ssid_name=, /ssid_pass=, /reset, /relay_in/off, /summer_time=1/0 "); }
    } 
  }

void timer0f() {
 if (millis() - timer0 >= timer0_interval) { 
 timer0 = millis(); 
 String disp_string ="";
 serial_comms();
 if (time_updated==false and WiFi.status()==WL_CONNECTED) {  
  if (timeClient.update() and time_updated==false) { time_updated=true; 
  if (debug==true) {Serial.println("Time updated from NTP!");}
  WiFi.mode(WIFI_OFF); }}   //wifi sleep
 go_time();
 battery_int=analogRead(bat_in);
 battery_voltage=(6.6/1023)*battery_int;
 countSensors = sensors.getDeviceCount();
 if (countSensors>0) {sensors.requestTemperatures(); temperature = sensors.getTempCByIndex(0); }
 lcd.clear();
 lcd.setCursor(0,0);
 if (hours<10) {disp_string=disp_string+"0"+String(hours);} else {disp_string=disp_string+String(hours);}  
 disp_string=disp_string+":"; 
 if (minutes<10) {disp_string=disp_string+"0"+String(minutes);} else {disp_string=disp_string+String(minutes);}
 disp_string=disp_string+":";
 if (seconds<10) {disp_string=disp_string+"0"+String(seconds);} else {disp_string=disp_string+String(seconds);}
 lcd.print(disp_string);   
 if (countSensors>0) {lcd.setCursor(10,0); lcd.print(String(temperature)+"C");}
 lcd.setCursor(0,1);
 if (battery_voltage<4.20) {
  lcd.print(String(days)+"/"+String(months)+"/"+String(years)+"|"+week[weakday]+"|"+String(battery_voltage));} else {
  lcd.print(String(days)+"/"+String(months)+"/"+String(years)+"|"+week[weakday]);} 
 if (battery_voltage<4.00) {lcd.noBacklight();} else {lcd.backlight();}
 if (battery_voltage<3.80) {lcd.setCursor(13,1); lcd.print("SLP"); lcd.noBacklight();}
 if (battery_voltage<3.75) {delay(200); ESP.deepSleep(0);}  
 //if (seconds>58 and minutes==44 and hours==3) {ESP.restart();}
 digitalWrite(led_in,0);
 byte r,g,b;
 //temperature
 if (temperature==-127){r=0; g=0; b=0;}
 if (temperature<0){r=0; g=0; b=127;}
 if (temperature>0){r=0; g=64; b=127;}
 if (temperature>4){r=0; g=127; b=127;}
 if (temperature>8){r=0; g=127; b=64;}
 if (temperature>12){r=0; g=127; b=0;}
 if (temperature>16){r=64; g=127; b=0;}
 if (temperature>20){r=127; g=127; b=0;} 
 if (temperature>24){r=127; g=64; b=0;}
 if (temperature>28){r=127; g=32; b=0;}
 if (temperature>30){r=127; g=0; b=0;} 
 if (temperature>32){r=127; g=0; b=64;}
 if (temperature>34){r=127; g=0; b=127;} 
 for (int i=0;  i<PixelCount; i++){
  strip.SetPixelColor(i, RgbColor(r,g,b));  
  }
 strip.Show();
 delay(100);
 digitalWrite(led_in,1); 
 }}

void setup()
{
  Serial.begin(9600);
  pinMode(led_in,OUTPUT);
  pinMode(zummer,OUTPUT);
  pinMode(bat_in,INPUT);
  lcd.init();                    
  lcd.setCursor(0,0);
  battery_int=analogRead(bat_in);
  battery_voltage=(6.6/1023)*battery_int;
  if (battery_voltage<3.85) {lcd.noBacklight(); lcd.print("SETUP...LOW POWER!"); ESP.deepSleep(0);}  //>else begin
  else {lcd.backlight();  
  WiFi.mode(WIFI_STA);
  //
  //write_string_EEPROM (lan_serv_adr,"http://192.168.0.208");
  lan_server_name=read_string_EEPROM(lan_serv_adr);
  //ssid_name="38E";
  //ssid_pass="S380965872552m";
  //writessid();
  readssid();
  WiFi.begin(ssid_name,ssid_pass);
  Serial.print("Conecting to "+ssid_name);
  while (WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }
  Serial.println("OK");
  summer_time=read_value_from_eeprom(eeprom_summer_time);
  sensors.begin();
  countSensors = sensors.getDeviceCount();
  if (summer_time==true) {timeClient.setTimeOffset(3600*(2+1));} else {timeClient.setTimeOffset(3600*2);}
  timeClient.begin(); 
  tone(zummer,1000,2000);
  strip.Begin();
  for (int i=0;  i<PixelCount; i++){
  strip.SetPixelColor(i, RgbColor(127,127,127));  
  }
  delay(1000);
  lcd.print("SETUP...OK!");}
}


void loop()
{
 timer0f();
}
