/*
    sqface - реализация алгоритма распознавания по методу Виолы-Джонса
    Автор: Александр Лубягин, lubyagin@yandex.ru
    Дата публикации: 06 декабря 2011 года
    Опубликовано на сайте SQFACE.RU под лицензией AGPLv3
    Текст лицензии: http://www.gnu.org/licenses/agpl-3.0.txt

    Данный проект использует также код под лицензией The MIT License
    в файлах:
    rapidxml.hpp
    rapidxml_iterators.hpp
    rapidxml_print.hpp
    rapidxml_utils.hpp
    (см. README.txt)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string.h>

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include "sqface.h"
#include "sqface_version.h"
#include "sqface_config.h"

#define max(a,b) ((a)>=(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define sqr(a) ((a)*(a))

#ifdef SQFACE_DEBUG
#define sqface_debug(...) printf(__VA_ARGS__)
#else
#define sqface_debug(...)
#endif


inline SUM_TYPE TFaceRecognizer::f_sum1(int x_s, int y_s, int w_r_scaled, int h_r_scaled) /* {{{ */
{
	SUM_TYPE S4 = -1;
	SUM_TYPE S1 = -1;
	SUM_TYPE S2 = -1;
	SUM_TYPE S3 = -1;

	if (y_s < 1) {
		S1 = 0;
		S2 = 0;
	}
	if (x_s < 1) {
		S1 = 0;
		S3 = 0;
	}

	S4 = p2[w2*(y_s+h_r_scaled-1) + (x_s+w_r_scaled-1)];
	if (S1 != 0) S1 = p2[w2*(y_s+0-1) + (x_s+0-1)];
	if (S2 != 0) S2 = p2[w2*(y_s+0-1) + (x_s+w_r_scaled-1)];
	if (S3 != 0) S3 = p2[w2*(y_s+h_r_scaled-1) + (x_s+0-1)];

	return (S4 + S1 - S2 - S3);
}
/* }}} */

inline SUM_TYPE2 TFaceRecognizer::f_sum2(int x_s, int y_s, int w_r_scaled, int h_r_scaled) /* {{{ */
{
	SUM_TYPE2 S4 = -1;
	SUM_TYPE2 S1 = -1;
	SUM_TYPE2 S2 = -1;
	SUM_TYPE2 S3 = -1;

	if (y_s < 1) {
		S1 = 0;
		S2 = 0;
	}
	if (x_s < 1) {
		S1 = 0;
		S3 = 0;
	}

	S4 = p3[w2*(y_s+h_r_scaled-1) + (x_s+w_r_scaled-1)];
	if (S1 != 0) S1 = p3[w2*(y_s+0-1) + (x_s+0-1)];
	if (S2 != 0) S2 = p3[w2*(y_s+0-1) + (x_s+w_r_scaled-1)];
	if (S3 != 0) S3 = p3[w2*(y_s+h_r_scaled-1) + (x_s+0-1)];

	return (S4 + S1 - S2 - S3);
}
/* }}} */

inline SUM_TYPE TFaceRecognizer::s_sum1(int x_s, int y_s, int w_r_scaled, int h_r_scaled) /* {{{ */
{
	SUM_TYPE S = 0;
	for (int y = y_s; y <= y_s+h_r_scaled-1; y++) {
		for (int x = x_s; x <= x_s+w_r_scaled-1; x++) {
			S += p1[stride1*y+bypp1*x];
		}
	}
	return S;
}
/* }}} */

inline SUM_TYPE2 TFaceRecognizer::s_sum2(int x_s, int y_s, int w_r_scaled, int h_r_scaled) /* {{{ */
{
	SUM_TYPE2 S = 0;
	for (int y = y_s; y <= y_s+h_r_scaled-1; y++) {
		for (int x = x_s; x <= x_s+w_r_scaled-1; x++) {
			S += sqr(p1[stride1*y+bypp1*x]);
		}
	}
	return S;
}
/* }}} */

inline float TFaceRecognizer::g_sum1(int x_s, int y_s, int w_r_scaled, int h_r_scaled) /* {{{ */
{
	float S = 0;
	for (int y = y_s; y <= y_s+h_r_scaled-1; y++) {
		for (int x = x_s; x <= x_s+w_r_scaled-1; x++) {
			S += p1[stride1*y+bypp1*x];
		}
	}
	return S/256.0;
}
/* }}} */

inline float TFaceRecognizer::g_sum2(int x_s, int y_s, int w_r_scaled, int h_r_scaled) /* {{{ */
{
	float S = 0;
	for (int y = y_s; y <= y_s+h_r_scaled-1; y++) {
		for (int x = x_s; x <= x_s+w_r_scaled-1; x++) {
			S += sqr(p1[stride1*y+bypp1*x]);
		}
	}
	return S/(256.0*256.0);
}
/* }}} */

// Конструктор
TFaceRecognizer::TFaceRecognizer() {
	FreeImage_Initialise(1);
}

// Деструктор
TFaceRecognizer::~TFaceRecognizer() {
  UnloadImage(); // на всякий случай
  FreeImage_DeInitialise();
}

// Загрузить изображение, и пред-обработать
int TFaceRecognizer::LoadImage(const char *filename_i) /* {{{ */
{
	// Оригинальное "цветное" изображение
	// , увеличить
	dib0 = FreeImage_Load(FreeImage_GetFIFFromFilename(filename_i), filename_i, JPEG_ACCURATE);

	w0 = FreeImage_GetWidth(dib0);
	h0 = FreeImage_GetHeight(dib0);
	p0 = FreeImage_GetBits(dib0);
	bpp0 = FreeImage_GetBPP(dib0);
	bypp0 = bpp0/8;
	stride0 = FreeImage_GetPitch(dib0); // что такое Pitch?
	dibo = FreeImage_Clone(dib0);

	dib1 = FreeImage_ConvertToGreyscale(dib0);
	FreeImage_FlipVertical(dib1);

	w1 = FreeImage_GetWidth(dib1);
	h1 = FreeImage_GetHeight(dib1);
	p1 = FreeImage_GetBits(dib1);
	bpp1 = FreeImage_GetBPP(dib1);
	bypp1 = bpp1/8;
	stride1 = FreeImage_GetPitch(dib1); // что такое Pitch?
	sqface_debug("bypp1 = %d\n", bypp1);

	// "Интегральная" матрица
	w2 = w1;
	h2 = h1; // no crop
	bypp2 = sizeof(SUM_TYPE); // of bytes
	bpp2 = bypp2*8; // bits
	stride2 = w2*bypp2; // unaligned
	p2 = (SUM_TYPE *)malloc(h2*stride2); // в байтах, (h2 x w2)
	if(!p2) {
		perror("No free memory.\n");
		exit(-1);
	}

	// "Интегральная" матрица
	w3 = w1;
	h3 = h1; // no crop
	bypp3 = sizeof(SUM_TYPE2); // of bytes
	bpp3 = bypp3*8; // bits
	stride3 = w3*bypp3; // unaligned
	p3 = (SUM_TYPE2 *)malloc(h3*stride3); // в байтах, (h3 x w3)
	if(!p3) {
		perror("No free memory.\n");
		exit(-1);
	}

	// Посчитать интегральное изображение
	// (однопоточный вариант алгоритма, по горизонтальным линиям)
	int x,y;
	p2[w2*0+0] = p1[stride1*0+bypp1*0];
	y = 0;
	for (x = 1; x < w2; x++) {
		p2[w2*y+x] = p2[w2*y+(x-1)] + p1[stride1*y+bypp1*x];
	}
	for (y = 1; y < h2; y++) {
		p2[w2*y+0] = p2[w2*(y-1)+0] + p1[stride1*y+bypp1*0]; // x == 0
		for (x = 1; x < w0; x++) {
			p2[w2*(y-0)+(x-0)] =
				p1[stride1*y+bypp1*x] +
				p2[w2*(y-1)+(x-0)] +
				p2[w2*(y-0)+(x-1)] -
				p2[w2*(y-1)+(x-1)];
		}
	}
	// "квадраты"
	p3[w3*0+0] = sqr(p1[stride1*0+bypp1*0]);
	y = 0;
	for (x = 1; x < w3; x++) {
		p3[w3*y+x] = p3[w3*y+(x-1)] + sqr(p1[stride1*y+bypp1*x]);
	}
	for (y = 1; y < h2; y++) {
		p3[w3*y+0] = p3[w3*(y-1)+0] + sqr(p1[stride1*y+bypp1*0]); // x == 0
		for (x = 1; x < w0; x++) {
			p3[w3*(y-0)+(x-0)] =
				sqr(p1[stride1*y+bypp1*x]) +
				p3[w3*(y-1)+(x-0)] +
				p3[w3*(y-0)+(x-1)] -
				p3[w3*(y-1)+(x-1)];
		}
	}
	// p0
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			sqface_debug(" %02X", p0[stride0*y+bypp0*x+2]);
		}
		sqface_debug("\n");
	}
	sqface_debug("\n");
	// p1
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			sqface_debug(" %02X", p1[stride1*y+bypp1*x+0]);
		}
		sqface_debug("\n");
	}
	sqface_debug("\n");
	// p1 (dec)
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			sqface_debug(" %3d", p1[stride1*y+bypp1*x+0]);
		}
		sqface_debug("\n");
	}
	sqface_debug("\n");

	// p2
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			sqface_debug(" %9d", p2[w2*y+x]);
		}
		sqface_debug("\n");
	}
	sqface_debug("\n");

	// p3
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			sqface_debug(" %9d", p3[w3*y+x]);
		}
		sqface_debug("\n");
	}
	return 0;
}
/* }}} */

using namespace std;
using namespace rapidxml;

int TFaceRecognizer::LoadCascadeXML(const char *filename_i_txt) /* {{{ */
{
	// Read file
	ifstream f(filename_i_txt);
	string xml;
	string line;
	while (getline(f,line)) xml += line;
	std::vector<char> xml_copy(xml.begin(), xml.end());
	xml_copy.push_back('\0');

	// Parse and print file
	xml_document<> doc;
	doc.parse<0>(&xml_copy[0]);

	// Cycles
	xml_node<> *node1 = doc.first_node("opencv_storage");
	if(!node1) return -1;
	xml_node<> *node2 = node1->first_node();//"haarcascade_frontalface_alt");
	if(!node2) return -1;

	xml_node<> *node3 = node2->first_node("size");
	if(!node3) return -1;
	char *p1;
	p1 = strtok(node3->value(), " "); this->cascade.window_w_mini = atoi(p1);
	p1 = strtok(0x00, " "); this->cascade.window_h_mini = atoi(p1);
	this->cascade.n_stages = 0;
	this->cascade.n_rects = 0;

	int i_stage = 0;
	int i_feature_abs = 0;
	int i_rect_abs = 0;

	xml_node<> *node4 = node2->first_node("stages");
	if(!node4) return -1;
	xml_node<> *node5 = node4->first_node("_");
	if(!node5) return -1;
	do {

		xml_node<> *node6 = node5->first_node("trees");
		if(!node6) {return -1;}

		xml_node<> *node9 = node5->first_node("stage_threshold");
		if(!node9) {return -1;}
		sqface_debug("stage_threshold %f\n", node9->value());
		float stage_threshold = atof(node9->value());

		xml_node<> *node10 = node5->first_node("parent");
		if(!node10) {return -1;}
		sqface_debug("parent %d\n", node10->value());

		xml_node<> *node11 = node5->first_node("next");
		if(!node11) {return -1;}
		sqface_debug("next %d\n", node11->value());

		this->stages[i_stage].n_features = 0;
		this->stages[i_stage].n_rects = 0;
		this->stages[i_stage].stage_threshold = stage_threshold;
		this->stages[i_stage].i_feature_abs_1 = 65535;
		this->stages[i_stage].i_feature_abs_2 = -1;

		xml_node<> *node7 = node6->first_node("_");
		if(!node7) {return -1;}

		xml_node<> *node8 = node7->first_node("_"); // tree ...
		if(!node8) {return -1;}
		do {

			xml_node<> *node15 = node8->first_node("feature");
			if(!node15) {return -1;}
			xml_node<> *node16 = node15->first_node("rects");
			if(!node16) {return -1;}
			xml_node<> *node17 = node16->first_node("_");

			xml_node<> *node12 = node8->first_node("threshold");
			if(!node12) {return -1;}
			float feature_threshold = atof(node12->value());

			float left_val = -1.0;
			float right_val = -1.0;
			xml_node<> *node13 = node8->first_node("left_val");
			if(!node13) {
				node13 = node8->first_node("left_node");
			}
			else {
				left_val = atof(node13->value());
			}

			xml_node<> *node14 = node8->first_node("right_val");
			if(!node14) {
				node14 = node8->first_node("right_node");
			}
			else {
				right_val = atof(node14->value());
			}

			this->features[i_feature_abs].i_stage = i_stage;
			//      this->features[i_feature_abs].n_rects = n_rects; // see rects
			this->features[i_feature_abs].feature_threshold = feature_threshold;
			this->features[i_feature_abs].left_val = left_val;
			this->features[i_feature_abs].right_val = right_val;
			if (this->stages[i_stage].i_feature_abs_1 > i_feature_abs)
				this->stages[i_stage].i_feature_abs_1 = i_feature_abs;
			if (this->stages[i_stage].i_feature_abs_2 < i_feature_abs)
				this->stages[i_stage].i_feature_abs_2 = i_feature_abs;
			this->features[i_feature_abs].i_rect_abs_1 = 65535;
			this->features[i_feature_abs].i_rect_abs_2 = -1;
			this->stages[i_stage].n_features = this->stages[i_stage].n_features + 1;


			if(!node17) {return -1;}
			do {
				int x,y,w,h;
				int weight;

				char *p2;
				p2 = strtok(node17->value(), " "); x = atoi(p2);
				p2 = strtok(0x00, " "); y = atoi(p2);
				p2 = strtok(0x00, " "); w = atoi(p2);
				p2 = strtok(0x00, " "); h = atoi(p2);
				p2 = strtok(0x00, " "); weight = (int)nearbyint(atof(p2));

				this->rects[i_rect_abs].x = x;
				this->rects[i_rect_abs].y = y;
				this->rects[i_rect_abs].w = w;
				this->rects[i_rect_abs].h = h;
				this->rects[i_rect_abs].weight = weight;
				if (this->features[i_feature_abs].i_rect_abs_1 > i_rect_abs)
					this->features[i_feature_abs].i_rect_abs_1 = i_rect_abs;
				if (this->features[i_feature_abs].i_rect_abs_2 < i_rect_abs)
					this->features[i_feature_abs].i_rect_abs_2 = i_rect_abs;
				this->features[i_feature_abs].n_rects = this->features[i_feature_abs].n_rects + 1;
				this->stages[i_stage].n_rects = this->stages[i_stage].n_rects + 1;

				this->cascade.n_rects = this->cascade.n_rects + 1;
				i_rect_abs = i_rect_abs + 1;

				node17 = node17->next_sibling("_");
				if(!node17) {break;}
			} while (1);

			i_feature_abs = i_feature_abs + 1;

			node7 = node7->next_sibling("_");
			if(!node7) {break;}
			node8 = node7->first_node("_");
			if(!node8) {break;}
		} while (1);

		this->cascade.n_stages = this->cascade.n_stages + 1;
		i_stage = i_stage + 1;

		node5 = node5->next_sibling("_");
		if(!node5) {break;}

	} while(1);
	return 0;
}
/* }}} */

// Записать изображение
int TFaceRecognizer::SaveImage(const char *filename_o) /* {{{ */
{
	//  FreeImage_FlipVertical(dib1);
	sqface_debug("Save: %s\n", filename_o);
	sqface_debug("w = %d\n", FreeImage_GetWidth(dib0));
	FreeImage_Save(FreeImage_GetFIFFromFilename(filename_o), dib0, filename_o, 0);
	return 0;
}
/* }}} */

// Выгрузить изображение
int TFaceRecognizer::UnloadImage() /* {{{ */
{
	free(p2);
	free(p3);
	FreeImage_Unload(dib0);
	FreeImage_Unload(dibo);
	FreeImage_Unload(dib1);
	return 0;
}
/* }}} */

// Распознать лица
int TFaceRecognizer::Recognize(float factor)
{
	for (int i_stage = 0; i_stage < cascade.n_stages; i_stage++) {
		sqface_debug("stage %d: %d rects\n", i_stage+1, this->stages[i_stage].n_rects);
	}
	clock_t t1 = clock();
	// Перевести в градации серого
	// Уменьшить картинку до других размеров, по этапам rescaling'а
	float dscale = 1.0;
	int i = 0;
	int window_w = cascade.window_w_mini;
	int window_h = cascade.window_h_mini;
	// Можно сделать scaling по-убывающей, с наибольших квадратов
	int count_window = 0;
#define MAX_W 120
	int a_ds[MAX_W]; // dscale
	unsigned long long count_rect_total = 0;
	unsigned long long count_rect_total_used = 0;
	int i_face = 1;
	do {
		unsigned long long count_rect = 0;
		for(int k = 0; k < MAX_W; k++) a_ds[k] = (int)floor(k*dscale);
		float sum_cascade = 0;
		i++;
		window_w = (int)floor(cascade.window_w_mini*dscale);
		window_h = (int)floor(cascade.window_h_mini*dscale);
		float inv = 1/float(window_w*window_h);
		int x1,y1,x2,y2;
		int x_step = max(1,min(4,window_w/10));
		int y_step = max(1,min(4,window_h/10));
		for (y1 = 0; y1 <= h1-1-window_h; y1+=y_step) {
			y2 = y1+window_h;
			for (x1 = 0; x1 <= w1-1-window_w; x1+=x_step) {
				x2 = x1+window_w;
				float mean = f_sum1(x1,y1,window_w,window_h)*inv;
				float variance = f_sum2(x1,y1,window_w,window_h)*inv - sqr(mean);
				float stddev = 1.0;
				if (variance > 0.0) stddev = sqrt(variance);
				if(stddev < 10.0) continue;
				// обработка "скользящего окна"
				int f_failed = 0; // хотя бы один этап "провалил"
				int f_passed = 0; // хотя бы один этап "прошел"
				for (int i_stage = 0; i_stage < cascade.n_stages; i_stage++) {
					float sum_stage = 0.0;
					for (int i_feature_abs = this->stages[i_stage].i_feature_abs_1;
							i_feature_abs <= this->stages[i_stage].i_feature_abs_2;
							i_feature_abs++) {
						int sum_feature = 0.0;
						for (int i_rect_abs = this->features[i_feature_abs].i_rect_abs_1;
								i_rect_abs <= this->features[i_feature_abs].i_rect_abs_2;
								i_rect_abs++) {
							int weight = (this->rects[i_rect_abs].weight);
							// перенес сюда - уменьшил 42 -> 28 sec
							int x_r_scaled = a_ds[this->rects[i_rect_abs].x];
							int y_r_scaled = a_ds[this->rects[i_rect_abs].y];
							int w_r_scaled = a_ds[this->rects[i_rect_abs].w];
							int h_r_scaled = a_ds[this->rects[i_rect_abs].h];
							int x_s = x1+x_r_scaled;
							int y_s = y1+y_r_scaled;
							sum_feature += (f_sum1(x_s,y_s,w_r_scaled,h_r_scaled)*weight);
							count_rect_total_used++;
						} // rects
						float leafth = this->features[i_feature_abs].feature_threshold * stddev;

						if (sum_feature*inv < leafth) sum_stage += this->features[i_feature_abs].left_val;
						else sum_stage += this->features[i_feature_abs].right_val;

						if (sum_stage > this->stages[i_stage].stage_threshold) {
							f_passed = 1;
							//break;
						}
					} // features
					sum_cascade += (float)sum_stage;
					if (sum_stage < this->stages[i_stage].stage_threshold) {
						f_failed = 1;
						break;
					}
				}
				if (f_failed == 0) {
					sqface_debug("%d %d %d %d: [%f]\n", x1,y1,x2,y2, stddev);
					if (stddev > 25.0) {
						int x,y;
						y = y1; for(x = x1; x <= x2; x++) {
							p0[stride0*(h0-y)+bypp0*x+0] = 0xFF;
							p0[stride0*(h0-y)+bypp0*x+1] = 0x3F;
							p0[stride0*(h0-y)+bypp0*x+2] = 0x3F;
						}
						y = y2; for(x = x1; x <= x2; x++) {
							p0[stride0*(h0-y)+bypp0*x+0] = 0xFF;
							p0[stride0*(h0-y)+bypp0*x+1] = 0x3F;
							p0[stride0*(h0-y)+bypp0*x+2] = 0x3F;
						}
						x = x1; for(y = y1; y <= y2; y++) {
							p0[stride0*(h0-y)+bypp0*x+0] = 0xFF;
							p0[stride0*(h0-y)+bypp0*x+1] = 0x3F;
							p0[stride0*(h0-y)+bypp0*x+2] = 0x3F;
						}
						x = x2; for(y = y1; y <= y2; y++) {
							p0[stride0*(h0-y)+bypp0*x+0] = 0xFF;
							p0[stride0*(h0-y)+bypp0*x+1] = 0x3F;
							p0[stride0*(h0-y)+bypp0*x+2] = 0x3F;
						}
						i_face++;
					}
				}
				count_rect += cascade.n_rects;
				count_window++;
			}
		}
		count_rect_total += count_rect;
		sqface_debug("%d: %d x %d, scale = %.4f; windows = %d; rects = %llu (%llu)\n",
				i,
				window_w,
				window_h,
				dscale,
				count_window,
				count_rect_total,
				count_rect_total_used
			  );

		dscale *= factor;
	} while(min(w1,h1) >= min(window_w,window_h));

	clock_t t2 = clock();
	sqface_debug("%.4f seconds\n", (t2-t1)/(double)(CLOCKS_PER_SEC));
	return 0;
}

int TFaceRecognizer::GetImageWidth() {
	return this->w0;
}

int TFaceRecognizer::GetImageHeight() {
	return this->h0;
}

