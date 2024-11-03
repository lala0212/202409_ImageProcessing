#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int width;
int height;
int colorDepth;
unsigned char header[54]; 
char filename_in[20]; 
int Size;
char filename[10];
int width_all;
float Gamma;
struct RGB
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
};

struct Ybr
{
	float Y;
	float Cb;
	float Cr;
};

struct Ybr RGB2Ybr(struct RGB RGB){
    struct Ybr Ybr;

    Ybr.Y = 0.257 * (float)RGB.R + 0.504 * (float)RGB.G + 0.098*(float)RGB.B + 16;
    Ybr.Cb = -0.148 * (float)RGB.R - 0.291*(float)RGB.G + 0.439*(float)RGB.B + 128;
    Ybr.Cr = 0.439 * (float)RGB.R - 0.368*(float)RGB.G - 0.071*(float)RGB.B + 128;
    return Ybr;
}
struct RGB Ybr2RGB(struct Ybr Ybr){
    struct RGB RGB;
    RGB.R = (unsigned char) fmax(0, fmin(255, 1.164 * (Ybr.Y - 16) + 1.596 * (Ybr.Cr - 128)));
    RGB.G = (unsigned char) fmax(0, fmin(255, 1.164 * (Ybr.Y - 16) - 0.392 * (Ybr.Cb - 128) - 0.813 * (Ybr.Cr - 128)));
    RGB.B = (unsigned char) fmax(0, fmin(255, 1.164 * (Ybr.Y - 16) + 2.017 * (Ybr.Cb - 128)));
    return RGB;
}
//read------------------------------------------------
unsigned char *read_bmp (const char *filename) {
    sprintf(filename_in, "%s.%s", filename, "bmp");
    FILE *fp;
    fp = fopen(filename_in, "rb"); //read binary    
    fread(header, sizeof(unsigned char), 54, fp);  // (暫存,讀取單位,讀取個數,檔名)
    width = *(int*)&header[18];// 從18往後讀4位元(int)
    height = *(int*)&header[22];// 從22往後讀4位元(int)
    colorDepth = *(int*)&header[28]/8;
    int rem = (width)%4;//padding prolbem
    width_all = width * colorDepth + rem;
    Size = width_all * height;
    unsigned char *data = (unsigned char*)malloc(Size);//動態分配
    fread(data, sizeof(unsigned char), Size, fp);
    fclose(fp);
    return data;
}
//Enhancement------------------------------------------------
void L_Enhancement_Gamma (unsigned char *data) {
    int index;
    for (int i = 0; i< height; i++){
        for (int j =0; j< width;j++){
            for (int k = 0; k<colorDepth;k++){
                //nonlinear2linear------------------------------------------------
                index = (i * width_all + j * colorDepth + k); 
                float tmp = (float)(data[index])/255.0;
                if (tmp<=0.03928){
                    tmp = (1/12.92)*tmp;
                }
                else{
                    tmp = pow((tmp+0.055)/1.055,2.4);
                }
                
                //Gamma correction------------------------------------------------
                tmp = pow(tmp,Gamma); 
                //linear2nonlinear------------------------------------------------
                if (tmp<=0.0031308){
                    tmp = 12.92*tmp;
                }
                else{
                    tmp = 1.055*pow(tmp,1/2.4)-0.055;
                }
                //8bit------------------------------------------------------------
                tmp = tmp*255.0;
                unsigned char tmp_out = (int)(tmp < 0 ? 0 : tmp > 255 ? 255 : tmp); 
                data[index] = tmp_out;
            }
        }
        //padding
        for (int j = width * colorDepth; j < width_all; j++) {
            data[i * width_all + j] = 0; 
        }
    }
    
}

void L_Enhancement_Y (unsigned char *data) {
    int index;
    struct RGB tmp;
    struct Ybr tmp2;
    float R,G,B,X,Y,Z,Cb,Cr; 
    for (int i = 0; i< height; i++){
        for (int j =0; j< width;j++){
            index = (i * width_all + j * colorDepth); 
            tmp.B = (float)(data[index+0]);
            tmp.G = (float)(data[index+1]);
            tmp.R = (float)(data[index+2]);
            tmp2 = RGB2Ybr(tmp);
            tmp2.Y = pow(tmp2.Y/235,Gamma)*235.0;
            tmp2.Y = (tmp2.Y < 16 ? 16 : tmp2.Y > 235 ? 235 : tmp2.Y); 
            tmp = Ybr2RGB(tmp2);
            data[index+0] = tmp.B;
            data[index+1] = tmp.G;
            data[index+2] = tmp.R;
        }
        //padding
        for (int j = width * colorDepth; j < width_all; j++) {
            data[i * width_all + j] = 0; 
        }
    }  
}
//write------------------------------------------------
void write_bmp( unsigned char *data){
    
    char filename_out[20]; 
    FILE *fp;
    sprintf(filename_out, "output%s.%s", filename+5, "bmp");
    fp = fopen(filename_out, "wb"); // 以二進位模式寫入檔案
    fwrite(header, sizeof(unsigned char), 54, fp); // 寫入 BMP 檔頭
    fwrite(data, sizeof(unsigned char), Size, fp); // 寫入圖像數據
    fclose(fp); // 關閉檔案
    free(data);
    //free------------------------------------------------
}

int main() {
    
    printf("input file name(ex: input1):");
    scanf("%s", filename); 
    // const char *filename = "input1";  
    unsigned char *data = read_bmp (filename);
    printf("Gamma: ");
    scanf("%f", &Gamma); 
    L_Enhancement_Y(data);
    write_bmp(data);
    return 0;
}
