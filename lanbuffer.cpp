#include <Arduino.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SPI.h>
#include <SD.h>
#include <chrono>
#include <ESP8266HTTPClient.h>

#define sd_cspin 15
#define power_sensor A0

template <class S>
class device_one_pin{
  protected:
  S pinIn;
  char pinMod; 
  int  digitalStatus, analogStatus, delayStatus = 0;
  bool enabled, digWrite, anWrite ,isAnalog ,isA0m = false;
  public:
  device_one_pin (S pin, char pMod = 'O', bool an = false, bool isA0 = false) {
    pinIn=pin; pinMod=pMod; isAnalog=an; isA0m=false;
    }
  bool enable(){
    if (isA0m==true) {pinIn=A0;}
    if (pinMod=='I') {pinMode(pinIn,INPUT); enabled=true;}
    if (pinMod=='P') {pinMode(pinIn,INPUT_PULLUP); enabled=true;}
    if (pinMod=='O') {pinMode(pinIn,OUTPUT); enabled=true;}
    return enabled;
    }    
  bool eStatus() {
    return enabled;   
    }
  int dStatus(){
    return digitalStatus;
    }
  int aStatus(){
    return analogStatus;
    }
  void SetDelay(bool state = false, int del = 100){
    digWrite=state;
    if (del>delayStatus) {delayStatus=del;}
    }  
  void DelayTick(){
    if (delayStatus>=0){
      delayStatus=delayStatus-1;
      digitalWrite(pinIn,digWrite);
      }      
    if (delayStatus==0){
      digWrite=!digWrite; 
      digitalWrite(pinIn,digWrite);
      }    
    }  
  void ReadPin() {
    digitalStatus=digitalRead(pinIn);
    analogStatus=analogRead(pinIn);
    }  
  void dWrite0(){  
    if (enabled==true){
      digWrite=0;
      digitalWrite(pinIn,digWrite);}
    }  
  void dWrite1(){ 
    if (enabled==true){
      digWrite=1;
      digitalWrite(pinIn,digWrite);}
    }  
  void dWrite(int bin){ 
    if (enabled==true){
      digWrite=bin;
      digitalWrite(pinIn,digWrite);}
    }
  void aWrite(int bin){  
    if (enabled==true){
      anWrite=bin;
      digitalWrite(pinIn,anWrite);}
    }
  void dWriteReverse(){   
    if (enabled==true){
      digWrite=!digWrite;
      digitalWrite(pinIn,digWrite);}
    } 
  void dWriteReverseAndDelay(int del){   
    if (enabled==true){
      digWrite=!digWrite;
      digitalWrite(pinIn,digWrite);
      delay(del); 
      }
    }     
  double backVolts (double nVolts){
    double vStatus = 0;
    if (enabled==true){vStatus=(nVolts/1024)*analogStatus;}
    return vStatus;
    }     
  };

template <class A>
class tTimer{
  protected:
  A (*fnc)() = NULL;
  unsigned long timer_interval, timer_now = 0;
  bool enabled = false;
  public:
  tTimer(unsigned long interval, bool enable = true, A (*cFcn)() = NULL){
    timer_interval=interval;
    enabled=enable;
    fnc=cFcn;
  }
  void Tick(){
    if ((micros() - timer_now > timer_interval) and (enabled==true)) { 
    timer_now = micros(); 
    fnc();   
    }
  }
  bool tEnabled(){
    return enabled;
  }
  void isEnable(bool en){
    enabled=en;
    }
  void setInterval(unsigned long i){
    timer_interval=i;
    }  
  }; 

class tFile {
  public:
  String recFileName = ""; 
  tFile(String fn = "0"){recFileName = fn;}
  bool initSD(int sd_pin = 0){
  if (sd_pin>0) {  
    return SD.begin(sd_pin);
    }
  return false;  
  }
  bool WriteToFile(String text = "", bool eol = true, int closeType = 1){
  File tFile;  
  if (closeType==-1) {tFile.close();}
  tFile = SD.open(recFileName, FILE_WRITE);
  if (tFile) {
    tFile.seek(EOF);
    if (eol==true) {tFile.println(text);} else {tFile.print(text);}
    if (closeType==1) {tFile.close();}
    return true;
    } else {
    return false;
    }  
  }
  bool ReWriteFile(String text = "", bool eol = true, int closeType = 1){
  File tFile;  
  if (closeType==-1) {tFile.close();}
  SD.remove(recFileName);
  tFile = SD.open(recFileName,FILE_WRITE);
  if (tFile) {
    if (eol==true) {tFile.println(text);} else {tFile.print(text);}
    if (closeType==1) {tFile.close();}
    return true;
    } else {
    return false;
    }  
  }
  String ReadFile(char f = ' ', String r = "" ,int closeType = 1){
  File tFile;  
  char t;
  String res = "";
  if (closeType==-1) {tFile.close();}
  tFile = SD.open(recFileName);
  if (tFile) {
    while (tFile.available()){
      t=tFile.read(); 
        if (t==f){
          res=res+String(t)+r;
          } else{
            res=res+String(t);
            }      
    } 
  if (closeType==1) {tFile.close();}   
  }
  return res;
  }
  bool CleanFile(int closeType = 1){
  File tFile;  
  if (closeType==-1) {tFile.close();}
  SD.remove(recFileName);
  tFile = SD.open(recFileName,FILE_WRITE);
  if (tFile) {
    if (closeType==1) {tFile.close();}
    return true;
    } else {
    return false;
    }  
  }
  bool accessToFile(){
  File recGFile;
  recGFile = SD.open(recFileName);
  if (recGFile) {
    recGFile.read();
    recGFile.close();
    return true;
    } else {
    return false;
    }  
  } 
};



//=========================================================================================================
  
 ESP8266WebServer server(80);
 WiFiUDP ntpUDP;
 NTPClient timeClient(ntpUDP, "ua.pool.ntp.org", 7200, 60000);
 String tts,ips = "";
 String active_web_page = "main_page";
 bool time_updated,sdError,manual_server_arg_access,last_power_status = false;
 int time_not_update = 5;
 int non_ntp_time = 0;
 int INTERNET = 10;
 unsigned long starting_unix_sytem_time = 0;
 unsigned long set_unix_system_time = 0;
 uint16_t timers[6] ={0,0,0,0,0,0};
 String  SSIDS = "";
 String  PASSS = "";
 String  lan_server_name = "";

//=========================================================================================================

void time_flow_0();
void time_flow_1();
void time_flow_2();
void time_flow_3();
void time_flow_4();
void autoInitFile();
void setup();
void loop();

//=========================================================================================================

tFile FileSystemLog("syslog.txt");
tFile FileWebLog("weblog.txt");
tFile FileUartLog("uartlog.txt");
tFile FileBuffer("buffer.txt");    
tFile FileNotepad("notepad.txt");
tFile CustomFile("nonExistFile.txt");
tTimer timer0(1,true,time_flow_0);
tTimer timer1(1*1000,true,time_flow_1);
tTimer timer2(1000*1000,true,time_flow_2); 
tTimer timer3(1000*1000,true,time_flow_3); 
tTimer timer4(3*1000*1000,true,time_flow_4);
device_one_pin led_in(2,'O',false); 
device_one_pin mic(A0,'I',true);
device_one_pin ps(power_sensor,'I',true,true);

//=========================================================================================================
  
String serial_comms(String hts = ""){
  if ((Serial.available() > 0) or (hts != "")) {
    hts=Serial.readString();   
    if (hts.substring(0,7)=="addlog=") {
    FileUartLog.WriteToFile(tts+">"+hts.substring(7,64));
    Serial.println(tts+">"+hts.substring(7,64));
    }
  led_in.SetDelay(false,1000);        
  }    
return hts;  
}

String add_button(String text = "", String link = ""){
  if (link!="") {
  text="<a href="+link+"><button type=\"button\">"+text+"</button></a>";
  } else {
    text="<button type=\"button\">"+text+"</button>";
    }
  return text;  
  }
String add_text_field(String label, String var_name ){
  String text = "";
/*  <label for="fname">First name:</label>
  <input type="text" id="fname" name="fname"><br><br> */
  text=text+"<form action=\"/\">";
  text=text+"<label for=\""+var_name+"\""+label+":</label>";
  text=text+"<input type=\"text\" id=\""+var_name+"\" name=\""+var_name+"\"><br><br>";
  text=text+"</form>";
  return text;    
}

bool What_page_active(String variant1 = "", String variant2 = "", String variant3 = "", String variant4 = "", String variant5 = ""){
  bool ret = false;
  if (server.arg(variant1)!="" or server.arg(variant2)!="" or server.arg(variant3)!="" or server.arg(variant4)!="" or server.arg(variant5)!=""){ret=true;}
  return ret;
  }

String html_response_form(String file_name, String dataquest, String questform, bool isSend = true, char h_find = '\n', String h_replace = "<br>"){
  String res,arg_data1 = "";
  res = "";
  if (file_name!=""){
  if (server.arg(dataquest)=="1" and questform=="get_data"){
    arg_data1=server.arg(dataquest);
    CustomFile.recFileName=file_name;
    res=CustomFile.ReadFile(h_find,h_replace,1)+" ";
  }
  if (server.arg(dataquest)!="" and questform=="put_data"){
    arg_data1=server.arg(dataquest);
    CustomFile.recFileName=file_name;
    CustomFile.WriteToFile(arg_data1); 
    res = " "; 
  }
  if (server.arg(dataquest)=="1" and questform=="clean_data"){
    arg_data1=server.arg(dataquest);
    CustomFile.recFileName=file_name;
    CustomFile.CleanFile();
    res=CustomFile.ReadFile(h_find,h_replace,1)+" "; 
  } 
  if (server.arg(dataquest)!="" and questform=="put_and_get_data"){
    arg_data1=server.arg(dataquest);
    CustomFile.recFileName=file_name;
    CustomFile.WriteToFile(arg_data1);
    res=CustomFile.ReadFile(h_find,h_replace,1);  
  }
  } else {
    res = "ERROR: Bad file name!";
    }  
return res;
}

String html_comms(){
  String res,file_name="";
  if (server.args()) {
    file_name=server.arg("file");
    if (What_page_active("addnote","wievnote","addwievnote","cleannote","0")){active_web_page="notepad_page";}
    if (What_page_active("addbuffer","wievbuffer","addwievbuffer","cleanbuffer","0")){active_web_page="buffer_page";}
    if (What_page_active("addfile","wievfile","addwievfile","cleanfile","0")){active_web_page="otherfile_page";}
    if (file_name==""){res=res+"<title>Local Buffer/Logger ["+active_web_page+"]</title>";} else {res=res+"<title>Local Buffer/Logger ["+file_name+"]</title>";}
    res=res+add_button("Wiew Notepad File","/?wievnote=1")+add_button("Wiew Buffer File","/?wievbuffer=1")+add_button("Wiew SystemLog File","/?file=syslog.txt&wievfile=1")+"<br>";
    res=res+html_response_form(FileNotepad.recFileName,"addnote","put_data");
    res=res+html_response_form(FileNotepad.recFileName,"wievnote","get_data");  
    res=res+html_response_form(FileNotepad.recFileName,"addwievnote","put_and_get_data");
    res=res+html_response_form(FileNotepad.recFileName,"cleannote","clean_data");
    
    res=res+html_response_form(FileBuffer.recFileName,"addbuffer","put_data");
    res=res+html_response_form(FileBuffer.recFileName,"wievbuffer","get_data");  
    res=res+html_response_form(FileBuffer.recFileName,"addwievbuffer","put_and_get_data");
    res=res+html_response_form(FileBuffer.recFileName,"cleanbuffer","clean_data"); 
  if (file_name!=""){
    res=res+html_response_form(file_name,"addfile","put_data");
    res=res+html_response_form(file_name,"wievfile","get_data");  
    res=res+html_response_form(file_name,"addwievfile","put_and_get_data");
    res=res+html_response_form(file_name,"cleanfile","clean_data");
    }
  if (active_web_page=="main_page") { 
    res=res+tts+"<br>";
    }
  if (active_web_page=="notepad_page") { 
    res=res+"<br>"+add_text_field("Input Data:","addwievnote")+"<br>";  
    res=res+add_button("Clear Notepad File","/?cleannote=1")+"<br>"; 
    res=res+tts+"<br>";    
    } 
  if (active_web_page=="buffer_page") { 
    res=res+"<br>"+add_text_field("Input Data:","addwievbuffer")+"<br>";  
    res=res+add_button("Clear Buffer File","/?cleanbuffer=1")+"<br>"; 
    res=res+tts+"<br>";     
    } 
  if (active_web_page=="otherfile_page") { 
    //res=res+"<br>"+add_text_field("Input Data:","file")+"<br>";
    //res=res+"<br>"+add_text_field("Input Data:","addwievfile")+"<br>";  
    res=res+add_button("Clear File","/?file="+file_name+"&cleanfile=1")+"<br>"; 
    res=res+tts+"<br>";    
    }       
  }
  if (res!=""){
    server.send(200,"text/html",res);   
    } else {
      led_in.SetDelay(false,1000);
      res=res+"<title>Local Buffer/Logger ["+active_web_page+"]</title>";
      res=res+add_button("Wiew Notepad File","/?wievnote=1")+add_button("Wiew Buffer File","/?wievbuffer=1")+add_button("Wiew SystemLog File","/?file=syslog.txt&wievfile=1")+"<br>";
      res=res+"Home Page";
      res=res+tts+"<br>";
      server.send(200,"text/html",res);
      }
  led_in.SetDelay(false,1000);       
  return res;
}

void get_main_page(){}

void led_flowing_down(int del = 1000){
  led_in.dWrite0();
  delay(del);
  led_in.dWrite1();
  delay(del);
  led_in.dWrite0();
  delay(del/2);
  led_in.dWrite1();
  delay(del/2);
  led_in.dWrite0();
  delay(del/3);
  led_in.dWrite1();
  delay(del/3);
  led_in.dWrite0();
  delay(del/4);
  }

void led_flowing_up(int del = 1000){
  led_in.dWrite0();
  delay(del/8);
  led_in.dWrite1();
  delay(del/7);
  led_in.dWrite0();
  delay(del/6);
  led_in.dWrite1();
  delay(del/5);
  led_in.dWrite0();
  delay(del/4);
  led_in.dWrite1();
  delay(del/3);
  led_in.dWrite0();
  delay(del/2);
  led_in.dWrite1();
  }  

String unixTimeToHumanReadable(long int seconds) //getted https://www.geeksforgeeks.org/convert-unix-timestamp-to-dd-mm-yyyy-hhmmss-format/
{
    String ans = "";
    int daysOfMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    long int currYear, daysTillNow, extraTime, extraDays, index, date, month, hours, minutes, secondss, flag = 0;
    daysTillNow = seconds / (24 * 60 * 60);
    extraTime = seconds % (24 * 60 * 60);
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
    // Calculating HH:MM:YYYY
    hours = extraTime / 3600;
    minutes = (extraTime % 3600) / 60;
    secondss = (extraTime % 3600) % 60;
    // Return the time
    ans = String(currYear)+"-"+String(month)+"-"+String(date)+"/"+String(hours)+":"+String(minutes)+":"+String(secondss);
    timers[5]=currYear;  
    timers[4]=month;
    timers[3]=date;
    timers[2]=hours;
    timers[1]=minutes;
    timers[0]=secondss; 
    return ans;
}


//?????????????????? ??????????
String send_and_get_mesage(String mes, String url_key){
 WiFiClient client; 
 HTTPClient http;
 String payload = "";
 int httpResponseCode = -1;
 String serverPath = "";
 serverPath = url_key+mes;
 Serial.println(serverPath);
 http.begin(client, serverPath.c_str());
 httpResponseCode = http.POST(serverPath);
 if (httpResponseCode>0) {
   Serial.print("HTTP Response code: ");
   Serial.println(httpResponseCode);
   payload = http.getString();
   Serial.println(payload);}
 else {
  Serial.print("Error code: ");
  Serial.println(httpResponseCode);}
 delay(100);
 http.end(); 
 return  payload;   
 }


void timecalculate_lan(){
  if (starting_unix_sytem_time==0){starting_unix_sytem_time = millis()/1000;}
  if (set_unix_system_time==0){set_unix_system_time=send_and_get_mesage("/?get=unix_time",lan_server_name).toInt();}  
  unsigned long res = (set_unix_system_time+(millis()/1000))-starting_unix_sytem_time;
  tts=unixTimeToHumanReadable(res); 
  if (set_unix_system_time>0 and time_updated==false){FileSystemLog.WriteToFile("//Time Set by Lan["+String(non_ntp_time)+"]:"+tts); time_updated=true;}
  Serial.println(String(res)); 
  Serial.println(tts);
}

//???????????? ?????????????? ???????? ???? ??????????????,???????????? ????????
void timecalculate_ntp(){
  if (INTERNET>0){INTERNET--;}
  if (time_updated==false and INTERNET>0) {time_updated=timeClient.update();} 
  unsigned long epochTime = timeClient.getEpochTime();
  tts=unixTimeToHumanReadable(epochTime); 
  if (INTERNET==1){
    FileSystemLog.WriteToFile("//Time Set by Ntp["+String(non_ntp_time)+"]:"+tts);
    time_updated=true;
    }  
  Serial.println(String(epochTime)); 
  Serial.println(tts);
  } 


void delemStrToTwoStr(String dstr, String * str1, String * str2, char delim){
  *str1=""; *str2="";
  String s1,s2 = "";
  int l = dstr.length(); 
  for (int i=0; i<l; i++){
      if (dstr[i]==delim){
        s1=dstr.substring(0,i-1); 
        s2=dstr.substring(i+1,l);
        break;
      }
  }
  *str1=s1; *str2=s2;
  Serial.println("");
  Serial.println("0sp:"+s1+"->"+s2);
}

//=========================================================================================================

void time_flow_0(){ //System task control flow 1mks
  server.handleClient();
  }

void time_flow_1(){ //System task control flow 1ms
  serial_comms();
  led_in.DelayTick();
  }

void time_flow_2(){ // SD card control flow 1sec
  
  if (FileSystemLog.accessToFile()) {            
  sdError=false;
  } else {
    led_flowing_down(1000);
    sdError=true;
    }
  if (sdError==true) ESP.restart(); 
  }

void time_flow_3(){ //integrated time control flow 1sec
 bool curr_power_status = false;
 ps.ReadPin();
 if (ps.backVolts(3.3*2)>4.5) {curr_power_status=true;}
 non_ntp_time++;
 if (lan_server_name!=""){
  timecalculate_lan();
 } else {
  timecalculate_ntp();
 }
 if (last_power_status!=curr_power_status){
   FileSystemLog.WriteToFile("//Change power status["+String(non_ntp_time)+"]["+tts+"]:"+String(curr_power_status)); 
   Serial.println("Change power status:"+String(curr_power_status));
   } 
 Serial.println(last_power_status);
 last_power_status=curr_power_status;  
 Serial.println(curr_power_status);
 Serial.println(ps.backVolts(3.3*2));
}

void time_flow_4(){ //Led indication 3sec
  led_in.SetDelay(false,100);
  }   

//=========================================================================================================

void setup() {
int brk = 1000;
Serial.begin(9600);
led_in.enable();
ps.enable();
FileSystemLog.initSD(sd_cspin);
FileSystemLog.WriteToFile("//System started");
CustomFile.recFileName="wifi_settings.ini";
delemStrToTwoStr(CustomFile.ReadFile(),&SSIDS,&PASSS,'\n');
CustomFile.recFileName="lan_time_server.ini";
lan_server_name=CustomFile.ReadFile();
time_flow_2();
WiFi.mode(WIFI_STA);
WiFi.begin(SSIDS,PASSS);      
Serial.println("");
while (WiFi.status() != WL_CONNECTED) {
  delay(50);
  led_in.dWrite0();
  delay(50);
  led_in.dWrite1(); 
  Serial.print(".");
  brk = brk-1;
  if (brk<0) break;
  }
if (brk>0){  
led_flowing_up(1000);  
Serial.println("");
Serial.print("Connected to ");
Serial.println(SSIDS);
Serial.print("IP address: ");
ips = String(WiFi.localIP()[0])+"."+String(WiFi.localIP()[1])+"."+String(WiFi.localIP()[2])+"."+String(WiFi.localIP()[3]);
Serial.println(ips);
FileSystemLog.WriteToFile(SSIDS);
FileSystemLog.WriteToFile(ips);    //manual_server_arg_access==true
bool curr_power_status = false;
ps.ReadPin();
if (ps.backVolts(3.3*2)>4.5) {curr_power_status=true;}
FileSystemLog.WriteToFile("//Change power status["+String(non_ntp_time)+"]["+tts+"]:"+String(curr_power_status)); 
last_power_status=curr_power_status;
server.onNotFound(get_main_page);
server.on("/", html_comms);
server.begin();
} else {
WiFi.mode(WIFI_OFF);
FileSystemLog.WriteToFile("WiFi is OFF!");
FileSystemLog.WriteToFile("AUTOMOUS MODE");  
}
}

void loop() { 
timer0.Tick();
timer1.Tick(); 
timer2.Tick();
timer3.Tick();
timer4.Tick();
}