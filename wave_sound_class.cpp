#include <iostream>
#include <fstream>
#include <string.h>
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
   uint16_t* wave_channel = 0;
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
        wave_channel = new uint16_t [s_rate];
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
        if (start_write_file==true){ bfile = fopen(fileName.c_str(),"ab");}
        wav_r_header.ChunkSize=(36+total_current_samples*(wav_f_header.BitsPerSample/8));
        wav_f_header.ByteRate=wav_f_header.SampleRate*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
        wav_f_header.BlockAlign=wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
        wav_d_header.Subchunk2Size=total_current_samples*wav_f_header.NumChannels*(wav_f_header.BitsPerSample/8);
        if (start_write_file==true){fseek(bfile,0,SEEK_SET);}
        fwrite(&wav_r_header,12,1,bfile);
        fwrite(&wav_f_header,24,1,bfile);
        fwrite(&wav_d_header,8,1,bfile); 
        //if (start_write_file==true){fseek(bfile,feof(bfile),SEEK_SET);}
        fwrite(wave_channel,current_sample*(wav_f_header.BitsPerSample/8),1,bfile); 
        fclose(bfile);
        start_write_file = true; 
        cout << total_current_samples << endl;
    }
    void import_from_wav_file(){
    
    }    
    void set_sample_rate(int s_rate){
        wav_f_header.SampleRate=s_rate;
    }

};



int main(){
    int input = 0;
    pcm_wave_audio wav(8000,600*8000);
    wav.generate_noise_buffer(8000*20);
    wav.export_to_wav_file("test3.wav");
  
    cin >> input;
    wav.generate_noise_buffer(8000*20);
    wav.export_to_wav_file("test3.wav");
    wav.delete_buffer();
    cin >> input;
    return 0;
}