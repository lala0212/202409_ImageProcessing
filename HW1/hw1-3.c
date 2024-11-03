#include <stdio.h>
#include <stdlib.h>

void Cropping_img(const char *filename) {
    int width;
    int height;
    int colorDepth;
    unsigned char header[54]; 
    //read------------------------------------------------
    char filename_in[20]; 
    sprintf(filename_in, "%s.%s", filename, "bmp");
    FILE *fp;
    fp = fopen(filename_in, "rb"); //read binary    
    fread(header, sizeof(unsigned char), 54, fp);  // (暫存,讀取單位,讀取個數,檔名)
    width = *(int*)&header[18];// 從18往後讀4位元(int)
    height = *(int*)&header[22];// 從22往後讀4位元(int)
    colorDepth = *(int*)&header[28]/8;
    int rem = (width)%4;//padding prolbem
    int width_all = width * colorDepth + rem;
    int Size = width_all * height;
    unsigned char *data = (unsigned char*)malloc(Size);//動態分配
    fread(data, sizeof(unsigned char), Size, fp);
    fclose(fp);
    //Cropping------------------------------------------------
    int x,y,w,h;
    printf("(x,y,w,h)：");
    while(getchar() != '\n');  // 消除換行符等多餘輸入
    scanf("(%d,%d,%d,%d)", &x, &y, &w, &h);
    if (x < 0 || y < 0 || (x + w) > width || (y + h) > height) {
        printf("無效的裁剪範圍。\n");
        free(data); 
        return;
    }
    rem = w%4;
    int w_all = w * colorDepth + rem;
    int Size2 = w_all * h;
    unsigned char *data2 = (unsigned char*)malloc(Size2);
    for (int i = 0; i < h; i++){
        for (int j =0; j < w;j++){
            for (int k = 0; k<colorDepth;k++){
                data2[i * w_all + j * colorDepth + k] = data[(i + y) * width_all + (j + x) * colorDepth + k];
            }
        }
        for (int j = w * colorDepth; j < w_all; j++) {
            data2[i * w_all + j] = 0; // padding
        }       
    }
    //write------------------------------------------------
    char filename_out[20]; 
    sprintf(filename_out, "output%s_crop.%s", filename+5, "bmp");
    fp = fopen(filename_out, "wb"); // 以二進位模式寫入檔案
    *(int*)&header[18] = w;
    *(int*)&header[22] = h;
    fwrite(header, sizeof(unsigned char), 54, fp); // 寫入 BMP 檔頭
    fwrite(data2, sizeof(unsigned char), Size2, fp); // 寫入圖像數據
    fclose(fp); // 關閉檔案

    //free------------------------------------------------
    free(data);
    free(data2);

}

int main() {
    char filename[10];
    printf("input file name(ex: input1):");
    scanf("%s", filename); 
    Cropping_img(filename);
    return 0;
}
