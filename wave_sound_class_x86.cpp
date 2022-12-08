#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
using namespace std;

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
  uint16_t BitsPerSample = 16;
  //uint16_t ExtraParamSize = 0;
};

struct data_header
{
  char Subchunk2ID[4] = {'d','a','t','a'}; 
  uint32_t Subchunk2Size = 0;
};

struct wave_data_frame
{
  uint16_t l_chanell = 0;
  uint16_t r_chanell = 0;
};


class pcm_wave_audio
{  
private:
   riff_header wav_r_header;
   format_header wav_f_header;
   data_header wav_d_header;
   int8_t* wave_channel = 0;
   bool start_write_file = false;
   int sr_duration = 0; //alldisc size
   int current_sample,total_current_samples = 0;
public:
    pcm_wave_audio(int srate, int buff_size){
        sr_duration=buff_size;
        wav_f_header.SampleRate=srate;
        create_buffer(sr_duration);
        clear_buffer();
    };
    ~pcm_wave_audio(){
        delete_buffer();
    };
    void create_buffer(int s_rate){
        wave_channel = new int8_t [s_rate];
    }
    void clear_buffer(){
        for (int i = 0; i < sr_duration; i++)
        {
            wave_channel[i]=0;
        } 
        current_sample = 0; 
    }
     void generate_noise_buffer(int duration){
        for (int i = 0; i < duration; i++)
        {
            wave_channel[i]=rand();
        } 
        current_sample = duration; 
        total_current_samples=total_current_samples+duration;
    } 
    void generate_tone_buffer(int duration, int freq, int amplified = 65535){
        double step,current_amp = 0;
        bool up_dn = true;
        wave_channel[0]=0;
        step = double(amplified)*(double(freq)/double(wav_f_header.SampleRate));
        for (int i = 0; i < duration; i++)
        {
            if (up_dn==true) {
                current_amp=current_amp+step;
                if (current_amp>amplified){
                    current_amp=amplified;
                    up_dn=false; 
                    }
                } else {
                current_amp=current_amp-step;
                if (current_amp<0){
                    current_amp=0;
                    up_dn=true; 
                    } 
                }
            
            wave_channel[i]=round(current_amp);
            //cout << wave_channel[i] << endl;
        } 
        current_sample = duration; 
        total_current_samples=total_current_samples+duration;
    } 
    void put_sample_data(int data, int adr = 0){
        if (adr==0){
            current_sample++;
            total_current_samples++;
            wave_channel[current_sample]=data;
        } else {
            wave_channel[adr]=data;
        }
    } 
    void recording_to_buffer(int sample_data){
        if (current_sample>=sr_duration){
            current_sample=0;
        }
        put_sample_data(sample_data);
    }

    int get_sample_data(int adr){
        return wave_channel[adr];
    } 
    void delete_buffer(){
        delete [] wave_channel;
    }
    void export_to_wav_file(string fileName){
        FILE *bfile;
        if (start_write_file==false){ bfile = fopen(fileName.c_str(),"wb");}
        if (start_write_file==true){ bfile = fopen(fileName.c_str(),"r+b");}
        wav_r_header.ChunkSize=(36+total_current_samples*(wav_f_header.BitsPerSample/8));
        wav_f_header.ByteRate=wav_f_header.SampleRate*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
        wav_f_header.BlockAlign=wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
        wav_d_header.Subchunk2Size=total_current_samples*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
        if (start_write_file==true){fseek(bfile,0,SEEK_SET);}
        fwrite(&wav_r_header,12,1,bfile);
        fwrite(&wav_f_header,24,1,bfile);
        fwrite(&wav_d_header,8,1,bfile); 
        if (start_write_file==true){fseek(bfile,0,SEEK_END);}
        fwrite(wave_channel,current_sample*(wav_f_header.BitsPerSample/8),1,bfile); 
        fclose(bfile);
        start_write_file = true; 
        cout << total_current_samples << endl;
    }
    void import_from_wav_file(){
    
    }    
    void import_from_dat_file(string fileName, int rate = 1000, int amplif = 1, int byte_per_frame = 1, int balance = -59){
        delete_buffer();
        FILE *bfile;
        int file_size,readed_size = 0;
        bfile = fopen(fileName.c_str(),"rb");
        fseek(bfile, 0, SEEK_END);
        file_size = ftell(bfile);
        create_buffer(file_size);
        fseek(bfile, 0, SEEK_SET);
        for (int i=0; i<file_size; i++) {
           readed_size=readed_size+fread(&wave_channel[i],byte_per_frame,1,bfile);
           wave_channel[i]=wave_channel[i]*amplif;
        }
        fclose(bfile);
        current_sample=file_size/byte_per_frame;
        total_current_samples=current_sample;
        wav_f_header.SampleRate=rate;
        wav_f_header.BitsPerSample=byte_per_frame*8;
        cout << readed_size << endl;
        cout << file_size << endl;
        cout << total_current_samples << endl;
    }     
    void set_sample_rate(int s_rate){
        wav_f_header.SampleRate=s_rate;
    }

};



int main(){
    int input,input2,s_rate,ampl,byte_per_frame,mic_balance = 0;
    int mode = -1;
    int tfreq = 50;
    string infn,outfn;
    input=3600;
    pcm_wave_audio wav(8000,input*8000);
    cout << "input program mode:" << endl;
    cout << "0=auto convert dat to wav" << endl;
    cout << "1=convert dat to wav" << endl;
    cout << "2=generate sounds" << endl;
    cout << "3=generate noise" << endl;
    cin >> mode;
    while (true)
    { 
      if (mode==0)
      {
        cout << "input dat filename, settings get from dattowav.ini" << endl;
        cin >> infn;
        ifstream tfile("dattowav.ini");
        tfile >> s_rate;
        tfile >> ampl;
        tfile >> byte_per_frame;
        tfile >> mic_balance;
        tfile.close();
        wav.import_from_dat_file(infn+".dat",s_rate,ampl,byte_per_frame,mic_balance);
        wav.export_to_wav_file(infn+".wav");
        cout << "--------------------------close program-------------------------------" << endl;
      }
      if (mode==1)
      {
        cout << "input dat filename, out wav filename, dat file samplerate, bytePerFrame, micBalance, amplitude" << endl;
        cin >> infn >> outfn >> s_rate >> byte_per_frame >> mic_balance >> ampl;
        wav.import_from_dat_file(infn+".dat",s_rate,ampl,byte_per_frame,mic_balance);
        wav.export_to_wav_file(outfn+".wav");
        cout << "---------------------------------------------------------" << endl;
      }
      if(mode==2)
      {
        cout << "input out filename, seconds for add tone sound to file, max=" << input << " seconds and frequence" << endl;
        cin >> outfn >> input2 >> tfreq;
        wav.generate_tone_buffer(input2*8000,tfreq);
        wav.export_to_wav_file(outfn+".wav");
        cout << "---------------------------------------------------------" << endl;
      }
      if(mode==3)
      {
        cout << "input out filename, seconds for add noise sound to file, max=" << input << " seconds" << endl;
        cin >> outfn >> input2;
        wav.generate_noise_buffer(input2*8000);
        wav.export_to_wav_file(outfn+".wav");
        cout << "---------------------------------------------------------" << endl;
      }
    }
    wav.delete_buffer();
    return 0;
}