#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int width;
int height;
int colorDepth;
unsigned char header[54]; 
char filename_in[20]; 
char filename[20];
int padding;
float varR,varG,varB;
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
//座標轉換---------------------------------------------------------------------
struct RGB Ybr2RGB(struct Ybr Ybr){
    struct RGB RGB;
    float tr = 1.164 * (Ybr.Y - 16) + 1.596 * (Ybr.Cr - 128);
    float tg = 1.164 * (Ybr.Y - 16) - 0.392 * (Ybr.Cb - 128) - 0.813 * (Ybr.Cr - 128);
    float tb = 1.164 * (Ybr.Y - 16) + 2.017 * (Ybr.Cb - 128);
    RGB.R = (unsigned char)fmax(0, fmin(255, tr));
    RGB.G = (unsigned char)fmax(0, fmin(255, tg));
    RGB.B = (unsigned char)fmax(0, fmin(255, tb));
    return RGB;
}
struct Ybr RGB2Ybr(struct RGB RGB){
    struct Ybr Ybr;
    Ybr.Y = 0.257 * (float)RGB.R + 0.504 * (float)RGB.G + 0.098*(float)RGB.B + 16;
    Ybr.Cb = -0.148 * (float)RGB.R - 0.291*(float)RGB.G + 0.439*(float)RGB.B + 128;
    Ybr.Cr = 0.439 * (float)RGB.R - 0.368*(float)RGB.G - 0.071*(float)RGB.B + 128;
    return Ybr;
}

//read------------------------------------------------
struct RGB **read_bmp (const char *filename) {
    sprintf(filename_in, "%s_1.%s", filename, "bmp");
    FILE *fp;
    fp = fopen(filename_in, "rb"); //read binary    
    fread(header, sizeof(unsigned char), 54, fp);  // (暫存,讀取單位,讀取個數,檔名)
    width = *(int*)&header[18];// 從18往後讀4位元(int)
    height = *(int*)&header[22];// 從22往後讀4位元(int)
    colorDepth = *(int*)&header[28]/8;
    padding = (4 - (width * colorDepth) % 4) % 4;
    // Allocate memory for 2D RGB array
    struct RGB **data = (struct RGB**)malloc(height * sizeof(struct RGB*));
    for (int i = 0; i < height; i++) {
        data[i] = (struct RGB*)malloc(width * sizeof(struct RGB));
    }

    // Read pixel data row by row
    for (int i = 0; i < height; i++) {
        fread(data[i], sizeof(struct RGB), width, fp);
        fseek(fp, padding, SEEK_CUR); // Skip padding
    }
    fclose(fp);
    return data;
}
void L_Enhancement_Y(struct RGB **data, float Gamma) {
    struct Ybr tmp2;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            tmp2 = RGB2Ybr(data[i][j]);
            // Y 分量增強 (L型增強)，使用 Gamma 校正
            tmp2.Y = pow(tmp2.Y / 235.0, Gamma) * 235.0;
            tmp2.Y = (tmp2.Y < 16.0) ? 16.0 : (tmp2.Y > 235.0) ? 235.0 : tmp2.Y;
            data[i][j] = Ybr2RGB(tmp2);
        }
    }
}
//write------------------------------------------------
void write_bmp( struct RGB **data,int index,int num){
    
    char filename_out[25]; 
    FILE *fp;
    sprintf(filename_out, "output%d_%d.%s", index, num,"bmp");
    fp = fopen(filename_out, "wb"); // 以二進位模式寫入檔案
    fwrite(header, sizeof(unsigned char), 54, fp); // 寫入 BMP 檔頭
    // Write pixel data row by row
    for (int i = 0; i < height; i++) {
        fwrite(data[i], sizeof(struct RGB), width, fp);    // Write pixel data
        for (int j = 0; j < padding; j++) {
            fputc(0, fp);                           // Write padding bytes
        }
    }
    fclose(fp); // 關閉檔案
    for (int i = 0; i < height; i++) {
        free(data[i]);
    }
    free(data);  
}

int main() {
    float g[4] = {0.7, 1.3, 1.1, 1.6};
    struct RGB **data;
    const char *filenames[4] = {"output1", "output2", "output3", "output4"};
    
    for (int i = 0; i < 4; i++) {
        char filename[30];  // 用來存儲完整檔案名稱
        data = read_bmp(filenames[i]); 
        L_Enhancement_Y(data, g[i]);
        write_bmp(data,i+1,2);
    }

    return 0;
}