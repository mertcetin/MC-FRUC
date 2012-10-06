/******************************************************************************
Copyright (c) 2007 Bart Adams (bart.adams@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software. The authors shall be
acknowledged in scientific publications resulting from using the Software
by referencing the ACM SIGGRAPH 2007 paper "Adaptively Sampled Particle
Fluids".

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
******************************************************************************/
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <iostream>
#include <fstream>
#include <string>
using namespace std;


class Config {
   public:

      Config() {}

      static void reload(string file);
      static void reload();

      
	  static int image_width;
	  static int image_height;
	  static int blocksize;
	  static int sadwindow;
	  static int total_cbcr;
	  static int total_y;
	  static int framecount;
	  static int uc_ratio;
	  static int fs_win;
	  static int bs_win;
	  static int vector_threshold;
	  static int SAD_threshold;
	  

	  static bool enable_rep;
	  static bool enable_new3DRS;
	  static bool first_fs;
	  static bool all_fs;
	  static bool bi_fs;
	  static bool enable_update;
	  static bool enable_early_t;
	  static int  occlusion_th;
	  static int  candidate_count;
	  static int  ext_candidate_count;
	  static int passcount;
	  static int up_conversion_algo;
	  static int search_location_list[];
	  static int ext_search_location_list[];

	  static float weights[];
	  static float spatial_weight;
	  static float temporal_weight;
	  static float update_weight;

	  static string video_size;
	  static string video_format;
	  static string input_video;
	  static string output_video;
	  static string MVs_filename;
	  static string Bi_MVs_filename;
	  static string MV_X_file;
	  static string MV_Y_file;
	  static string results_file;
	  
	  static string _file;

      
};
#endif
