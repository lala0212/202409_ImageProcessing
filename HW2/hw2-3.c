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
//noise_estamate---------------------------------------------------------------------
void noise_estamate(struct RGB **data) {
    int index = 0;
    float avgr = 0, avgb = 0, avgg = 0;
    int dev = 1;
    int len = (height / dev-2) * (width / dev-2);
    float *Jr = (float*)malloc(len * sizeof(float));
    float *Jb = (float*)malloc(len * sizeof(float));
    float *Jg = (float*)malloc(len * sizeof(float));
    // n +=1; 
    // Populate Jr, Jb, and Jg arrays with differences
    for (int i = 1; i < height / dev-2; i++) {
        for (int j = 1; j < width / dev-2; j++) {
            Jr[index] = data[i][j].R - (1.0 / 4.0) * (
                data[i + 1][j].R +
                data[i - 1][j].R +
                data[i][j + 1].R +
                data[i][j - 1].R);

            Jb[index] = data[i][j].B - (1.0 / 4.0) * (
                data[i + 1][j].B +
                data[i - 1][j].B +
                data[i][j + 1].B +
                data[i][j - 1].B);

            Jg[index++] = data[i][j].G - (1.0 / 4.0) * (
                data[i + 1][j].G +
                data[i - 1][j].G +
                data[i][j + 1].G +
                data[i][j - 1].G);
        }
    }
    
    // Calculate average for each color channel
    for (int i = 0; i < len; i++) {
        avgr += Jr[i];
        avgb += Jb[i];
        avgg += Jg[i];
    }
    avgr /= (float)len;
    avgb /= (float)len;
    avgg /= (float)len;
    
    // Calculate variance for R channel
    float diffR = 0;
    float diffG = 0;
    float diffB = 0;
    for (int i = 0; i < len; i++) {
        diffR += (Jr[i] - avgr)*(Jr[i] - avgr);
        diffG += (Jg[i] - avgg)*(Jg[i] - avgg);
        diffB += (Jb[i] - avgb)*(Jb[i] - avgb);
    }
    diffR /=(float)len;
    diffG /=(float)len;
    diffB /=(float)len;
    varR = 4.0 / 5.0 * diffR;
    varG = 4.0 / 5.0 * diffG;
    varB = 4.0 / 5.0 * diffB;
    free(Jr);
    free(Jb);
    free(Jg);

}
int gaussian(int **data) {
    float Mask[3][3] = {{1.0f / 16, 2.0f / 16, 1.0f / 16},
                        {2.0f / 16, 4.0f / 16, 2.0f / 16},
                        {1.0f / 16, 2.0f / 16, 1.0f / 16}};
    
    float tmp = 0.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            tmp += data[i][j] * Mask[i][j];
        }
    }

    // 將結果限制在 0 到 255 的範圍內
    int result = (int)(tmp < 0 ? 0 : (tmp > 255 ? 255 : tmp));
    return result;
}

int Bilateral(int **data, int n){//n是矩陣邊長
    double d = 150.0;//space
    double r = 50.0;//color
    double c[n][n];
    double s[n][n];
    int center = n / 2;
    double sum = 0;
    double sums = 0;
    double tmp = 0;
    double h;
    for (int i = -center; i <= center; i++) {
        for (int j = -center; j <= center; j++) {
            int x_ = center + i;
            int y_ = center + j;
            c[x_][y_] = exp(-pow(sqrt(i * i + j * j), 2)/ (2*d*d));
            s[x_][y_] = exp(-pow(abs(data[x_][y_] -data[center][center]),2)/ (2*r*r));
            tmp += data[x_][y_]*(c[x_][y_]*s[x_][y_]);
            sum +=  c[x_][y_]*s[x_][y_];
        }
    } 
    h = tmp/sum;
    return (int)h;
}

//adaptive_noise_reduction-----------------------------------------------------------------------
int adaptive_noise_reduction(int **data, int n, float vare) {//n是寬
    int center = n / 2;
    int g = data[center][center];
    int len = n * n;
    int S_xy[len];
    int index = 0;
    float avg = 0;
    float varxy = 0;
    
    // Populate S_xy with neighborhood values
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            avg += data[i][j];
        }
    }
    avg /= (float)len;
    // Calculate variance of S_xy
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            varxy += (data[i][j]-avg)*(data[i][j]-avg);
        }
    }
    varxy /= (float)len;
    if (varxy == 0) varxy = 1e-5; // Avoid division by zero
    int result = (int)((float)g - (vare / varxy) * ((float)g - avg));

    // Clamp result to 0–255
    if (result < 0) result = 0;
    if (result > 255) result = 255;
    return result;
}




//adaptive_median_filter-----------------------------------------------------------------------
int adaptive_median_filter(int **data, int S_max) {
    int S_xy = 3;
    int center = S_max / 2;
    int tmp[S_max * S_max];
    float z_med;
    while (S_xy <= S_max) {
        int index = 0;
        int range = S_xy / 2;
       
        // 收集 S_xy x S_xy 區域內的像素值到 tmp 陣列
        for (int i = -range; i <= range; i++) {
            for (int j = -range; j <= range; j++) {
                tmp[index++] = data[center + i][center + j];
            }
        }

        // 對 tmp 陣列按 Y 值進行泡沫排序
        for (int i = 0; i < S_xy * S_xy; i++) {
            for (int j = i + 1; j < S_xy * S_xy; j++) {
                if (tmp[i] > tmp[j]) {
                    int temp = tmp[i];
                    tmp[i] = tmp[j];
                    tmp[j] = temp;
                }
            }
        }

        // 計算 z_min, z_max, z_med 和 z_xy
        float z_min = tmp[0];
        float z_max = tmp[S_xy * S_xy - 1];
        z_med = tmp[(S_xy * S_xy) / 2];
        float z_xy = data[center][center];

        // Level A 判斷
        if (z_min < z_med && z_med < z_max) {
            // Level B 判斷
            if (z_min < z_xy && z_xy < z_max) {
                return z_xy;  // 符合條件，返回原值
            } else {
                return z_med;  // 返回中值
            }
        } else {
            S_xy += 2;  // 增加鄰域大小
        }
    }
    return z_med;  // 若達到 S_max，輸出中值
}

//MedianFilter-----------------------------------------------------------------------
int MedianFilter(int **data, int n) {
    int flattened[n * n];
    int index = 0;

    // Flatten the 2D array
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            flattened[index++] = data[i][j];
        }
    }

    // Bubble sort for the single channel
    for (int i = 0; i < n * n; i++) {
        for (int j = i + 1; j < n * n; j++) {
            if (flattened[i] > flattened[j]) {
                int temp = flattened[i];
                flattened[i] = flattened[j];
                flattened[j] = temp;
            }
        }
    }

    // Return the median value
    return flattened[n * n / 2];
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
void Denoise(struct RGB **data,int n, int filter) {
    noise_estamate(data);
    struct RGB tmp;
    struct RGB **padded_data = (struct RGB**) malloc((height + 2*n) * sizeof(struct RGB*));
    for (int i = 0; i < height + 2*n; i++) {
        padded_data[i] = (struct RGB*)calloc(width + 2*n, sizeof(struct RGB)); // 將所有欄位 (Y, Cb, Cr) 初始化為 0
    }
    // 將原始資料複製到 padding 陣列的中心
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            padded_data[i + n][j + n] = data[i][j];
        }
    }
    //padding
    for (int i = 0; i < n; i++) {
        memcpy(padded_data[i], padded_data[n], (width + 2 * n) * sizeof(struct RGB));
        memcpy(padded_data[height + n + i], padded_data[height + n - 1], (width + 2 * n) * sizeof(struct RGB));
    }
    for (int i = 0; i < height + 2 * n; i++) {
        for (int j = 0; j < n; j++) {
            padded_data[i][j] = padded_data[i][n];
            padded_data[i][width + n + j] = padded_data[i][width + n - 1];
        }
    }
    
    //calculate variance 
    

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int **R_channel_data = (int **)malloc((2 * n + 1) * sizeof(int *));
            int **G_channel_data = (int **)malloc((2 * n + 1) * sizeof(int *));
            int **B_channel_data = (int **)malloc((2 * n + 1) * sizeof(int *));
            for (int k = 0; k < 2 * n + 1; k++) {
                R_channel_data[k] = (int *)malloc((2 * n + 1) * sizeof(int));
                G_channel_data[k] = (int *)malloc((2 * n + 1) * sizeof(int));
                B_channel_data[k] = (int *)malloc((2 * n + 1) * sizeof(int));
            }

            // 提取每個通道的鄰域數據
            for (int di = -n; di <= n; di++) {
                for (int dj = -n; dj <= n; dj++) {
                    R_channel_data[n + di][n + dj] = padded_data[i + n + di][j + n + dj].R;
                    G_channel_data[n + di][n + dj] = padded_data[i + n + di][j + n + dj].G;
                    B_channel_data[n + di][n + dj] = padded_data[i + n + di][j + n + dj].B;
                }
            }
            // 獲取中值濾波後的值
            if (filter == 1){//MedianFilter
                tmp.R = MedianFilter(R_channel_data, 2 * n + 1);
                tmp.G = MedianFilter(G_channel_data, 2 * n + 1);
                tmp.B = MedianFilter(B_channel_data, 2 * n + 1);
            }
            else if (filter == 2){//adaptive_median_filter
                tmp.R = adaptive_median_filter(R_channel_data, 2 * n + 1);
                tmp.G = adaptive_median_filter(G_channel_data, 2 * n + 1);
                tmp.B = adaptive_median_filter(B_channel_data, 2 * n + 1);
            }
            else if (filter == 3){//Sharpness
                tmp.R = gaussian(R_channel_data);
                tmp.G = gaussian(G_channel_data);
                tmp.B = gaussian(B_channel_data);
            }
            else if (filter == 4){
               tmp.R = Bilateral(R_channel_data, 2 * n + 1);
               tmp.G = Bilateral(G_channel_data, 2 * n + 1);
               tmp.B = Bilateral(B_channel_data, 2 * n + 1);
            }
            else if (filter == 5){
               tmp.R = adaptive_noise_reduction(R_channel_data, 2 * n + 1,varR);
               tmp.G = adaptive_noise_reduction(G_channel_data, 2 * n + 1,varG);
               tmp.B = adaptive_noise_reduction(B_channel_data, 2 * n + 1,varB);
            }
            // 轉換回 RGB 並儲存到原始資料陣列中
            data[i][j] = tmp;
            for (int k = 0; k < 2 * n + 1; k++) {
                free(R_channel_data[k]);
                free(G_channel_data[k]);
                free(B_channel_data[k]);
            }
            free(R_channel_data);
            free(G_channel_data);
            free(B_channel_data);       
        }
    }
    // 釋放 padded_data 陣列的記憶體
    for (int i = 0; i < height + 2*n; i++) {
        free(padded_data[i]);
    }
    free(padded_data);

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
    int index;
    printf("input:(3/4):");
    scanf("%d", &index); 
    if (index == 3){
        strcpy(filename, "input3");
        struct RGB **data = read_bmp (filename);
        Denoise(data,1,1);
        write_bmp(data,1);
        data = read_bmp (filename);
        Denoise(data,2,2);
        write_bmp(data,2);
    }
    else{
        strcpy(filename, "input4");
        struct RGB **data2 = read_bmp (filename);
        Denoise(data2,20,5);//20
        Denoise(data2,1,3);
        Denoise(data2,1,1);
        write_bmp(data2,1);
        data2 = read_bmp (filename);
        Denoise(data2,4,4);
        write_bmp(data2,2);

    }
    
    return 0;
}
