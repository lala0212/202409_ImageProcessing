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
    RGB.R = (unsigned char) tr>255?255:tr<0?0:tr;
    RGB.G = (unsigned char) tg>255?255:tg<0?0:tg;
    RGB.B = (unsigned char) tb>255?255:tb<0?0:tb;
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

//Denoise Median_filter------------------------------------------------
void MaxRGB(struct RGB **data) {
    unsigned char maxR = 0;
    unsigned char maxG = 0;
    unsigned char maxB = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (data[i][j].R > maxR) maxR = data[i][j].R;
            if (data[i][j].G > maxG) maxG = data[i][j].G;
            if (data[i][j].B > maxB) maxB = data[i][j].B;
        }
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            data[i][j].R = data[i][j].R*(255.0/maxR);
            data[i][j].B = data[i][j].B*(255.0/maxB);
            data[i][j].G = data[i][j].G*(255.0/maxG);
        }
    }
}
void Grayworld(struct RGB **data) {
    unsigned int sumR = 0, sumG = 0, sumB = 0;
    float N = width * height;  // 總像素數

    // 計算 RGB 的總和
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            sumR += data[i][j].R;
            sumG += data[i][j].G;
            sumB += data[i][j].B;
        }
    }

    // 計算平均值
    float avgR = sumR / N;
    float avgG = sumG / N;
    float avgB = sumB / N;
    float K = (avgR + avgG + avgB) / 3.0;  // 灰世界假設的中性值

    // 調整 RGB 值
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            data[i][j].R = (unsigned char)fmin(255, fmax(0, data[i][j].R * K / avgR));
            data[i][j].G = (unsigned char)fmin(255, fmax(0, data[i][j].G * K / avgG));
            data[i][j].B = (unsigned char)fmin(255, fmax(0, data[i][j].B * K / avgB));
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
    int index; // 將 index 改為普通變數
    printf("input: ");
    scanf("%s", filename); // 輸入檔案名稱
    struct RGB **data = read_bmp(filename); // 讀取 BMP 檔案

    printf("method: \n1. MaxRGB\n2. GrayWorld\n");
    scanf("%d", &index); // 輸入方法選擇

    if (index == 1) {
        MaxRGB(data); // 執行 MaxRGB 方法
    } else if (index == 2) {
        Grayworld(data); // 執行 GrayWorld 方法
    } else {
        printf("Invalid method selected!\n"); // 無效選擇
    }

    write_bmp(data, 1); // 寫入輸出檔案
    return 0;
}
