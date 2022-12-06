#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

struct bmp_file_header
{
      //file header 14bytes
      uint8_t bftype[2] = {'B','M'};
      uint32_t bfsize = 0;
      uint16_t reserved[2] = {0,54};
      uint32_t offbits = 0;
};

struct bmp_img_header
{
      //bitmap header 40bytes
      uint32_t bisize = 40;
      uint32_t biwidth = 0;
      uint32_t biheigth = 0;
      uint16_t biplanes = 1;  //16
      uint16_t bibitcount = 24;  //16
      uint32_t bicompression = 0;
      uint32_t bisizeimage = 0; //image size
      uint32_t bixpels = 0;
      uint32_t biypels = 0;
      uint32_t biclrused = 0;
      uint32_t biclrimportant = 0;
};

struct tcolor
{
     //colors format
     uint8_t r = 0;
     uint8_t g = 0;
     uint8_t b = 0;
     uint8_t a = 0;
};


class picture
{
private:
    int pixels_x, pixels_y = 0;
    tcolor** image_map = 0; 
    bmp_file_header bmp_fh;
    bmp_img_header bmp_ih;
public:
    picture(int x = 1, int y = 1, int r = 119, int g = 120, int b = 121, string import_image_fname = ""){      
      if (import_image_fname==""){
        pixels_x=x;
        pixels_y=y;
        create_image_map();
        clear_image_map(r,g,b);
      } else {
        pixels_x=1;
        pixels_y=1;
        create_image_map();
        import_from_bmp_file(import_image_fname);
      }
    };
    ~picture(){
       delete_image_map(); 
    };
    void create_image_map(){
      image_map = new tcolor* [pixels_y+1];
      for (int i=0; i<pixels_y+1; i++){
        image_map[i] = new tcolor [pixels_x+1]; 
      }
    }
    int import_from_bmp_file(string fileName){
      int results=0;
      uint8_t reserve = 0;
        FILE *bfile;
        bfile = fopen(fileName.c_str(),"rb");
        fread(&bmp_fh,14,1,bfile);
        fread(&bmp_ih,40,1,bfile);
        reset_image(bmp_ih.biwidth,bmp_ih.biheigth,0,0,0);
        int bisize_check = 0;
        for (int a=0; a<pixels_y+1; a++){
            for (int b=0; b<pixels_x+1; b++)
            {
             if (bisize_check++>=(bmp_ih.bisizeimage)) break; 
             fread(&image_map[a][b].r,1,1,bfile); 
             if (bisize_check++>=(bmp_ih.bisizeimage)) break;
             fread(&image_map[a][b].g,1,1,bfile);
             if (bisize_check++>=(bmp_ih.bisizeimage)) break;
             fread(&image_map[a][b].b,1,1,bfile);
            }
        }
        fclose(bfile); 
      results=1;
      return results;
    }    
    int export_to_bmp_file(string fileName, int bit_count_for_pixel = 24){
      int results,bytes_writen=0;
        uint8_t reserve = 0;
        if (bmp_ih.bisizeimage==0) {bmp_ih.bisizeimage = (pixels_y*pixels_x*(bit_count_for_pixel/8));} 
        bmp_ih.biwidth=pixels_x;
        bmp_ih.biheigth=pixels_y;
        bmp_ih.bibitcount=bit_count_for_pixel;
        FILE *bfile;
        bfile = fopen(fileName.c_str(),"wb");
        fwrite(&bmp_fh,14,1,bfile);
        fwrite(&bmp_ih,40,1,bfile);
        bytes_writen=0; 
        for (int a=0; a<pixels_y; a++){
            for (int b=0; b<pixels_x+1; b++)
            {
            if (bit_count_for_pixel==24){
             if (bytes_writen++>=(bmp_ih.bisizeimage)) break;
             fwrite(&image_map[a][b].r,1,1,bfile); 
             if (bytes_writen++>=(bmp_ih.bisizeimage)) break;
             fwrite(&image_map[a][b].g,1,1,bfile);
             if (bytes_writen++>=(bmp_ih.bisizeimage)) break;
             fwrite(&image_map[a][b].b,1,1,bfile);  
            } 
            if (bit_count_for_pixel==8){
             if (bytes_writen++>=(bmp_ih.bisizeimage)) break;
             fwrite(&image_map[a][b].a,1,1,bfile);   
            }             
            }
        }
        fclose(bfile); 
      results=1;
      return results;
    }    
    void clear_image_map(int red, int green, int blue){
      for (int a=0; a<pixels_y; a++){
        for (int b=0; b<pixels_x; b++){
          image_map[a][b].r = red; 
          image_map[a][b].g = green;
          image_map[a][b].b = blue; 
        } 
      }
    }
    uint32_t get_byte_image_size(){
        return bmp_ih.bisizeimage;
    }
    void set_byte_image_size(uint32_t size){
        bmp_ih.bisizeimage=size;
    }
    uint8_t get_red_color_by_index(int x, int y){
      return image_map[y][x].r;
    }
    uint8_t get_green_color_by_index(int x, int y){
      return image_map[y][x].g;
    }
    uint8_t get_blue_color_by_index(int x, int y){
      return image_map[y][x].b;
    }
    void set_color_by_index(int x, int y, int r, int g, int b){
      image_map[y][x].r=r;
      image_map[y][x].g=g;
      image_map[y][x].b=b;
    }   
    void set_xy(int x, int y){
      pixels_x=x;
      pixels_y=y;
    } 
    uint32_t get_max_x(){
      return pixels_x;
    }  
    uint32_t get_max_y(){
      return pixels_y;
    }    
    void reset_image(int x, int y, int r, int g, int b){
      if (image_map>0) {delete_image_map();}
      pixels_x=x;
      pixels_y=y;
      create_image_map();
      clear_image_map(r,g,b);
    }    
    void delete_image_map(){
      for (int i=0; i<pixels_y; i++){
        delete [] image_map[i]; 
      }    
      delete [] image_map; 
    }
};

void text_image_editor(){
    string ifn,ofn,finput = "";
    int c=120;
    picture pic1;
    picture pic2;
    while (true){
    cout << "input 'o' for open, 'm' for merge, 'c' for create blank image, 'w' for wiev, 'e' for edit, 's' for save:" << endl;
    cin >> finput; 
    if (finput=="o"){
      cout << "input file name for open:" << endl;
      cin >> ifn;
      pic1.import_from_bmp_file(ifn+".bmp");         
    } 
    if (finput=="m"){
      int x = 10;
      int y = 10;
      cout << "input file name for open:" << endl;
      cin >> ifn;
      pic1.import_from_bmp_file("aa.bmp"); 
      cout << "input merge file name for open:" << endl;
      cin >> ifn;
      cout << "input currX currY for merging" << endl;
      cin >> x >> y;  
      pic2.import_from_bmp_file("tt.bmp");
      tcolor color;
      for (int a=0; a<pic2.get_max_x(); a++){
        for (int b=0; b<pic2.get_max_y(); b++){
          color.r=(pic1.get_red_color_by_index(a+x,b+y)+pic2.get_red_color_by_index(a,b))/2;
          color.g=(pic1.get_green_color_by_index(a+x,b+y)+pic2.get_green_color_by_index(a,b))/2;
          color.b=(pic1.get_blue_color_by_index(a+x,b+y)+pic2.get_blue_color_by_index(a,b))/2;
          pic1.set_color_by_index(a+x,b+y,color.r,color.g,color.b);
        } 
      } 
      pic1.export_to_bmp_file("test.bmp",24);        
    } 
    if (finput=="t"){
      int x,y,r,g,b;
      cout << "test functions mode" << endl;    
      pic1.reset_image(640,480,0,0,0);
      pic1.export_to_bmp_file("test.bmp",24); 
    } 
    if (finput=="c"){
      int x,y,r,g,b;
      cout << "input maxX maxY red green blue for basic creating image" << endl;
      cin >> x >> y >> r >> g >> b;    
      pic1.reset_image(x,y,r,g,b);
    }
    if (finput=="w"){    
    cout << "---------------------------------------------------" << endl;
    for (int a=0; a<pic1.get_max_y(); a++){
        for (int b=0; b<pic1.get_max_y(); b++){
          cout << pic1.get_red_color_by_index(a,b) << "." << pic1.get_green_color_by_index(a,b) << "." << pic1.get_blue_color_by_index(a,b) << '\t';  
        } 
        cout << endl;
      }
    cout << "---------------------------------------------------" << endl;      
    } 
    if (finput=="e"){    
      int x,y,r,g,b;
      cout << "input curX curY red green blue for change pixel" << endl;
      cin >> x >> y >> r >> g >> b;      
    }  
    if (finput=="s"){    
    cout << "input file name for save:" << endl;
    cin >> ofn;     
    pic1.export_to_bmp_file(ofn+".bmp");      
    } 
    }
    pic1.delete_image_map();
}

int main(){
    int input = 0;
    text_image_editor();
    cin >> input;
    return 0;
}