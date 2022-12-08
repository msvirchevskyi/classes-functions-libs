

#define led_add 5
#define mic A5    //A5 - A1
#define serial_bal_set false

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>

struct riff_header
{
  char ChunkID[4] = {'R','I','F','F'};
  uint32_t ChunkSize = 0; //DATA SIZE + HEADER
  char 	Format[4] = {'W','A','V','E'};
};

struct format_header
{
  char Subchunk1ID[4] = {'f','m','t',' '};
  uint32_t Subchunk1Size = 16;
  uint16_t AudioFormat = 1;
  uint16_t NumChannels = 1;
  uint32_t SampleRate = 8000;
  uint32_t ByteRate = 0;
  uint16_t BlockAlign = 0;
  uint16_t BitsPerSample = 8;
  //uint16_t ExtraParamSize = 0;
};

struct data_header
{
  char Subchunk2ID[4] = {'d','a','t','a'}; 
  uint32_t Subchunk2Size = 0;
};

File wav;
String fn = "";
int nf,rec_time = 1;
int hzrate = 4000;
int delms = (1000000/hzrate)*0.4;
int micb = 270;  //59 - 270
int micdiv = 3;
int buff_size = 600;
int c_frame = 0;
//uint8_t val[400];
uint8_t* val = 0;


void addHeaderToFile(String lfn){
    File cwav;
    riff_header wav_r_header;
    format_header wav_f_header;
    data_header wav_d_header;
    fn=lfn+".wav";
    cwav = SD.open(lfn.c_str(),FILE_READ);
    wav = SD.open(fn.c_str(),FILE_WRITE);
    wav_f_header.SampleRate=hzrate;
    wav_r_header.ChunkSize=(36+cwav.size()*(wav_f_header.BitsPerSample/8));
    wav_f_header.ByteRate=wav_f_header.SampleRate*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
    wav_f_header.BlockAlign=wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
    wav_d_header.Subchunk2Size=cwav.size()*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
    //writing header
    wav.write(wav_r_header.ChunkID);
    wav.write(wav_r_header.ChunkSize);
    wav.write(wav_r_header.Format);
    //---
    wav.write(wav_f_header.Subchunk1ID);
    wav.write(wav_f_header.Subchunk1Size);
    wav.write(wav_f_header.AudioFormat);
    wav.write(wav_f_header.NumChannels);
    wav.write(wav_f_header.SampleRate);  
    wav.write(wav_f_header.ByteRate);
    wav.write(wav_f_header.BlockAlign);
    wav.write(wav_f_header.BitsPerSample);
    //---
    wav.write(wav_d_header.Subchunk2ID);
    wav.write(wav_d_header.Subchunk2Size);
    //---
    cwav.close();
    wav.close();
    //---
    for(int i=0; i<wav_d_header.Subchunk2Size; i++){
    cwav = SD.open(lfn.c_str(),FILE_READ);
    wav = SD.open(fn.c_str(),FILE_WRITE);
    cwav.seek(i);
    wav.seek(EOF);
    wav.write(cwav.read());
    cwav.close();
    wav.close();
    }
    //---
    
}



void setup() {
  pinMode(led_add,OUTPUT);
  pinMode(mic,INPUT);
  Serial.begin(9600);
  digitalWrite(led_add,true);
  //delay(1000);
  Serial.print("Initializing SD card...");
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1); }
  Serial.println("initialization done.");
  if (SD.exists("settings.ini")){
    File f_set;
    f_set=SD.open("settings.ini",FILE_READ);
    hzrate=f_set.readString().toInt();
    //micb=f_set.readString().toInt();
    delms = (1000000/hzrate)*0.4;
    f_set.close();
    Serial.println("settings loaded");
  }
  /*
  if (SD.exists("lastfile.ini")){
    File f_set;
    String lpf;
    f_set=SD.open("lastfile.ini",FILE_READ);
    lpf=f_set.readString();
    f_set.close();
    Serial.print("rebuild:"+lpf+"...");
    if (SD.exists(lpf.c_str())){
      addHeaderToFile(lpf); 
      Serial.println("OK");
      }
    SD.remove("lastfile.ini");
  }
  */
  if (serial_bal_set==false){
  fn="rec"+String(nf)+".dat";
  while (SD.exists(fn.c_str())) {nf=nf+1; fn="rec"+String(nf)+".dat"; delay(1);} 
    Serial.println("writing to "+fn+"...");
    Serial.println("MIC BALANCE="+String(micb));
    Serial.println("RATE="+String(hzrate));
    digitalWrite(led_add,false);
    delay(500);
    digitalWrite(led_add,true);
    delay(1000);
    /*
    File f_set;
    f_set=SD.open("lastfile.ini",FILE_WRITE);
    f_set.seek(0);
    f_set.print(fn);
    f_set.close(); 
    */
    digitalWrite(led_add,false);
  }
  val = new uint8_t [buff_size]; 
}

void loop() {
for (int i=1; i<buff_size; i++){
  c_frame = ((analogRead(mic)-micb)/micdiv);
  if (c_frame>abs(micb*3)){c_frame=0;}
  analogWrite(led_add,abs(c_frame));
  val[i]=c_frame+127;
  if (c_frame+127>255){val[i]=255;}
  if (c_frame+127<1){val[i]=0;}
  if (serial_bal_set==true) {Serial.println(String(c_frame)+"|"+String(val[i]));}
  delayMicroseconds(delms);
}
if (serial_bal_set==false){
  wav = SD.open(fn.c_str(),FILE_WRITE);
  wav.seek(EOF);
  wav.write(val,buff_size);
  wav.close();
  val[0]=val[buff_size-1];
}

}