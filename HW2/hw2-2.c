#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int width;
int height;
int colorDepth;
unsigned char header[54]; 
char filename_in[20]; 
char filename[10];
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
    RGB.R = (unsigned char)(tr > 255 ? 255 : (tr < 0 ? 0 : tr));
    RGB.G = (unsigned char)(tg > 255 ? 255 : (tg < 0 ? 0 : tg));
    RGB.B = (unsigned char)(tb > 255 ? 255 : (tb < 0 ? 0 : tb));
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
    sprintf(filename_in, "%s.%s", filename, "bmp");
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
float Mask[3][3]={{0,0,0},{0,1,0},{0,0,0}};
float Mask1[3][3]={{-1,-1,-1},{-1,9,-1},{-1,-1,-1}};
float Mask2[3][3] = {
    {-1/16.0, -2/16.0, -1/16.0},
    {-2/16.0, 12/16.0, -2/16.0},
    {-1/16.0, -2/16.0, -1/16.0}
};
//Denoise Median_filter------------------------------------------------
void Sharpness(struct RGB **data,int filter) {
    int n = 1;
    struct Ybr **padded_data = (struct Ybr**) malloc((height + 2*n) * sizeof(struct Ybr*));
    for (int i = 0; i < height + 2*n; i++) {
        padded_data[i] = (struct Ybr*)calloc(width + 2*n, sizeof(struct Ybr)); // 將所有欄位 (Y, Cb, Cr) 初始化為 0
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            padded_data[i + n][j + n] = RGB2Ybr(data[i][j]);
        }
    }
    //padding
    for (int i = 0; i < n; i++) {
        memcpy(padded_data[i], padded_data[n], (width + 2 * n) * sizeof(struct Ybr));
        memcpy(padded_data[height + n + i], padded_data[height + n - 1], (width + 2 * n) * sizeof(struct Ybr));
    }
    for (int i = 0; i < height + 2 * n; i++) {
        for (int j = 0; j < n; j++) {
            padded_data[i][j] = padded_data[i][n];
            padded_data[i][width + n + j] = padded_data[i][width + n - 1];
        }
    }
    
    for (int i = 0; i <height; i++) {
        for (int j = 0; j <width; j++) {
            float tmp = 0;
            struct Ybr data_tmp;
            for (int di = -n; di <=n; di++) {
                for (int dj = -n; dj <=n; dj++) {
                    if (filter ==1){
                        tmp += padded_data[n+i+di][n+j+dj].Y*Mask1[n+di][n+dj];
                    }
                    else{
                        tmp += padded_data[n+i+di][n+j+dj].Y*Mask[n+di][n+dj]+15*padded_data[n+i+di][n+j+dj].Y*Mask2[n+di][n+dj];
                    }
                }
            }
            data_tmp.Y = tmp;
            data_tmp.Cb = padded_data[n+i][n+j].Cb;
            data_tmp.Cr = padded_data[n+i][n+j].Cr;
            data[i][j] = Ybr2RGB(data_tmp);
        }
    }   
 
}

//write------------------------------------------------
void write_bmp( struct RGB **data,int num){
    char filename_out[20]; 
    FILE *fp;
    sprintf(filename_out, "output%s_%d.%s", filename+5, num,"bmp");
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
    strcpy(filename, "input2");
    struct RGB **data = read_bmp (filename);
    Sharpness(data,1);
    write_bmp(data,1);
    data = read_bmp (filename);
    Sharpness(data,2);
    write_bmp(data,2);
    return 0;
}
