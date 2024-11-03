#include <stdio.h>
#include <stdlib.h>

void Resolution_img(const char *filename) {
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
    colorDepth = *(int*)&header[28]/8;//只考慮24跟32
    int rem = (width)%4;//padding prolbem
    int width_all = width * colorDepth + rem;
    int Size = width_all * height;
    unsigned char *data = (unsigned char*)malloc(Size);//動態分配
    fread(data, sizeof(unsigned char), Size, fp);
    fclose(fp);
    //Resolution------------------------------------------------
    int index;
    int n = 256;
    for (int fig_case =0;fig_case<3;fig_case++ ){
        n = n/4;
        for (int i = 0; i< height; i++){
            for (int j =0; j< width;j++){
                for (int k = 0; k<colorDepth;k++){
                    index = i * width_all + j * colorDepth + k;
                    data[index] = (data[index]/(256/n))*255/(n-1);
                }
            }
            //padding
            for (int j = width * colorDepth; j < width_all; j++) {
                data[i * width_all + j] = 0; 
            }
        //write------------------------------------------------
        char filename_out[20]; 
        sprintf(filename_out, "output%s_%d.%s", filename+5,fig_case+1, "bmp");
        fp = fopen(filename_out, "wb"); // 以二進位模式寫入檔案
        fwrite(header, sizeof(unsigned char), 54, fp); // 寫入 BMP 檔頭
        fwrite(data, sizeof(unsigned char), Size, fp); // 寫入圖像數據
        fclose(fp); // 關閉檔案
        }
    }

    
    

    //free------------------------------------------------
    free(data);

}

int main() {
    char filename[10];
    printf("input file name(ex: input1):");
    scanf("%s", filename); 
    Resolution_img(filename);
    return 0;
}
