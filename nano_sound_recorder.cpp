

#define led_add 5
#define mic A5    //A5 - A1

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <math.h>

struct riff_header       //12bytes
{
  char ChunkID[4] = {'R','I','F','F'};       
  uint32_t ChunkSize = 0; //DATA SIZE + HEADER
  char 	Format[4] = {'W','A','V','E'};
};

struct format_header     //24bytes
{
  char Subchunk1ID[4] = {'f','m','t',' '};
  uint32_t Subchunk1Size = 16;
  uint16_t AudioFormat = 1;
  uint16_t NumChannels = 1;
  uint32_t SampleRate = 0;
  uint32_t ByteRate = 0;
  uint16_t BlockAlign = 0;
  uint16_t BitsPerSample = 8;
  //uint16_t ExtraParamSize = 0;
};

struct data_header      //8bytes
{
  char Subchunk2ID[4] = {'d','a','t','a'}; 
  uint32_t Subchunk2Size = 0;
};

File wav;
bool non_stop_record_mode = false;   //activate auto start recording
String fn = "";                      //file name
uint16_t micb = 270;                 //hardware microphone balance
int8_t micdiv = 3;                   //noise levels
uint8_t micact = 48;                 //minimal mirophone level for start recording
int16_t buff_size = 600;             //wav buffer size
uint8_t* val = 0;                    //wav bufer array
unsigned long writing_points = 0;
unsigned long all_frames = 0;
unsigned long used_micros = 0;
uint16_t hzrate = 4000;
int16_t delms = (1000000/hzrate);


void addHeaderToFile(){
  riff_header wav_r_header;
  format_header wav_f_header;
  data_header wav_d_header;
  wav = SD.open(fn.c_str(),FILE_WRITE);
  wav_f_header.SampleRate=hzrate;
  wav_r_header.ChunkSize=(36+all_frames*(wav_f_header.BitsPerSample/8));
  wav_f_header.ByteRate=wav_f_header.SampleRate*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
  wav_f_header.BlockAlign=wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
  wav_d_header.Subchunk2Size=all_frames*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
  //writing header
  wav.seek(0);
  wav.write(reinterpret_cast<char*>(&wav_r_header),12);
  wav.write(reinterpret_cast<char*>(&wav_f_header),24);
  wav.write(reinterpret_cast<char*>(&wav_d_header),8);
  wav.close();
  //--- 
}

void add_data_to_file(){
  wav = SD.open(fn.c_str(),FILE_WRITE);
  wav.seek(EOF);
  wav.write(val,buff_size);
  wav.close();
}

bool file_is_exist(){        //standard library function not accept String().c_str();
  File rf = SD.open(fn.c_str(),FILE_READ);
  bool res = false;
  if (rf!=0){res=true;}
  rf.close();
  return res;
}


void setup() {
  uint16_t nf = 0;                     //file number
  pinMode(led_add,OUTPUT);
  pinMode(mic,INPUT);
  Serial.begin(9600);
  digitalWrite(led_add,true);
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(10)) {
    Serial.println(F("initialization failed!"));
    while (1); }
  Serial.println(F("initialization done."));
  fn="rec"+String(nf)+".wav";
  while (file_is_exist()) {
    nf++; 
    fn="rec"+String(nf)+".wav"; 
    delay(1);
    } 
    Serial.print(F("writing to "));
    Serial.print(fn);
    Serial.println(F("..."));
    Serial.print(F("MIC BALANCE="));
    Serial.println(String(micb));
    Serial.print(F("RATE="));
    Serial.println(String(hzrate));
    delay(1000);
    digitalWrite(led_add,false);
  val = new uint8_t [buff_size]; 
}

void loop() {
int16_t c_frame = 0; 
unsigned long last_micros = 0;
delms = (1000000/hzrate);
delms = delms - used_micros/buff_size;    
if (delms<1){delms=0;}
for (int i=1; i<buff_size; i++){
  c_frame = analogRead(mic)-micb;
  c_frame = (c_frame) / micdiv;
  analogWrite(led_add,abs(c_frame));
  val[i]=c_frame+127;
  if (c_frame+127>255){val[i]=255;}
  if (c_frame+127<1){val[i]=0;}
  //Serial.println(String(c_frame)+"|"+String(val[i]));      //debug
  if (c_frame>micact){
    writing_points=20000;
  }
  if (writing_points>0){writing_points--;}
  delayMicroseconds(delms);
}
if (writing_points>0 or non_stop_record_mode==true){
  all_frames=all_frames+buff_size;
  last_micros=micros();
  add_data_to_file();        //20millis
  addHeaderToFile();         //40millis
  used_micros=micros();
  used_micros=used_micros-last_micros;
  //Serial.println(String(used_micros)+"-"+String(delms));   //debug
} 
val[0]=val[buff_size-1];
}