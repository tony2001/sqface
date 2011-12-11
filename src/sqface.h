
#include <FreeImage.h>
#include <stdio.h>  // printf,perror
#include <stdlib.h> // abs
#include <time.h>   // clock
#include <string.h> // strncmp
#include <math.h>   // floor

// ~10-25
#define MAX_STAGES 100
// ~1-5 тыс (max)
#define MAX_FEATURES 10000
// ~5-10 тыс (max)
#define MAX_RECTS 30000

// 32-bit

#define SUM_TYPE signed int
#define SUM_TYPE2 signed int

typedef struct {
  int window_w_mini, window_h_mini;
  int n_stages;
  int n_rects;
} TXMLCascade;

typedef struct {
  int n_features;
  int n_rects;
  float stage_threshold;
  int i_feature_abs_1;
  int i_feature_abs_2;
} TStage;

typedef struct {
  int i_stage;
  int n_rects;
  float left_val;
  float right_val;
  float feature_threshold;
  int i_rect_abs_1;
  int i_rect_abs_2;
} TFeature;

typedef struct {
  int i_stage;
  int i_feature;
  int i_rect;
  int x,y;
  int w,h;
  int weight;
} TRect;

struct TFace {
  int x1;
  int y1;
  int x2;
  int y2;
  int f; // признак
};



// Основан на каскадах Хаара
class TFaceRecognizer {
private:
  // "Цветная" картинка
  FIBITMAP *dib0;
  WORD w0,h0;
  BYTE *p0;
  WORD bpp0;
  WORD bypp0;
  WORD stride0;
  FIBITMAP *dibo; // draw

  // "Серая" картинка
  FIBITMAP *dib1;
  WORD w1,h1;
  BYTE *p1;
  WORD bpp1;
  WORD bypp1;
  WORD stride1;

  // "Интегральная" матрица
  WORD w2,h2;
  SUM_TYPE *p2; // not BYTE *
  WORD bpp2;
  WORD bypp2;
  WORD stride2;

  // "Интегральная" матрица из "квадратов"
  WORD w3,h3;
  SUM_TYPE2 *p3; // not BYTE *
  WORD bpp3;
  WORD bypp3;
  WORD stride3;

  // Каскад Хаара
  const char *filename_i_txt;
  TXMLCascade cascade;
  TStage stages[MAX_STAGES];
  TFeature features[MAX_FEATURES];
  TRect rects[MAX_RECTS];

public:
#define START_FACES 2000
  TFaceRecognizer(); // Конструктор
  ~TFaceRecognizer(); // Деструктор
  int LoadImage(const char *filename_i); // Загрузить изображение
  int LoadCascadeXML(const char *filename_i); // Загрузить каскад Хаара в XML-формате
  int SaveImage(const char *filename_o); // Записать изображение
  int UnloadImage(); // Выгрузить изображение
  int Recognize(float factor); // Распознать лица
  int GetImageWidth();
  int GetImageHeight();
  inline SUM_TYPE f_sum1(int x_s, int y_s, int w_r_scaled, int h_r_scaled);
  inline SUM_TYPE2 f_sum2(int x_s, int y_s, int w_r_scaled, int h_r_scaled);
  inline SUM_TYPE s_sum1(int x_s, int y_s, int w_r_scaled, int h_r_scaled);
  inline SUM_TYPE2 s_sum2(int x_s, int y_s, int w_r_scaled, int h_r_scaled);
  inline float g_sum1(int x_s, int y_s, int w_r_scaled, int h_r_scaled);
  inline float g_sum2(int x_s, int y_s, int w_r_scaled, int h_r_scaled);
};

