#include "config.h"

int Config::image_width				= 176;
int Config::image_height				= 144;
int Config::blocksize				= 8;
int Config::sadwindow				= 8;
int Config::total_cbcr				= 12672;
int Config::total_y					= 25344;
int Config::framecount				= 400;
int Config::uc_ratio					= 2;
int Config::fs_win					= 32;
int Config::bs_win					= 4;
int Config::vector_threshold			= 3;
int Config::SAD_threshold			= 1000;

bool Config::enable_rep				= true;
bool Config::enable_early_t			= true;
bool Config::enable_new3DRS			= false;
bool Config::first_fs				= false;
bool Config::all_fs					= false;
bool Config::bi_fs					= false;
bool Config::enable_update			= true;
int  Config::occlusion_th			= 3;

int Config::candidate_count			= 5;
int Config::ext_candidate_count		= 12;
int Config::passcount				= 1;
int Config::up_conversion_algo		= 1;

int Config::search_location_list[50];
int Config::ext_search_location_list[50];

float Config::weights[] = {1.0,1.0,1.0};

float Config::spatial_weight	= 1.0;
float Config::temporal_weight	= 1.0;
float Config::update_weight		= 1.0;

string Config::video_size			= "CIF";
string Config::video_format			= "400";
string Config::input_video			= "foremanY.yuv";
string Config::output_video			= "output.yuv";
string Config::MVs_filename			= "MVs.txt";
string Config::Bi_MVs_filename		= "Bi_MVs.txt";
string Config::MV_X_file				= "MV_X.txt";
string Config::MV_Y_file				= "MV_Y.txt";
string Config::results_file			= "results.txt";

string Config::_file="";

   void Config::reload() {
      reload(_file);
   }

   void Config::reload(string file) {
         _file = file;
         ifstream input(file.c_str(), ios::in);
         string param;
         while (input >> param) {
         if (param == string("IMG_SIZE"))
		 {
			 string temp;
			 input >> temp;
			 if (temp == "QCIF")
			 {
				 image_width = 176;
				 image_height = 144;
				 total_y	= 25344;
			 } 
			 else if ( temp == "CIF")
			 {
				 image_width = 352;
				 image_height = 288;
				 total_y	= 101376;
			 }
			 else if (temp == "SIF")
			 {
				 image_width = 352;
				 image_height = 240;
				 total_y	= 84480;
			 }
			 else if (temp == "4CIF")
			 {
				 image_width = 704;
				 image_height = 576;
				 total_y = 405504;
			 }
			 else if (temp == "4SIF")
			 {
				 image_width = 704;
				 image_height = 460;
				 total_y = 323840;
			 }
			 else if (temp == "576p")
			 {
				 image_width = 720;
				 image_height = 576;
				 total_y = 414720;
			 }
			 else if (temp == "720p")
			 {
				 image_width = 1280;
				 image_height = 720;
				 total_y = 921600;
			 }
			 else if (temp == "1080p")
			 {
				 image_width = 1920;
				 image_height = 1080;
				 total_y = 2073600;
			 }
		 }
		 else if (param == string("IMG_HEIGHT")) input >> image_height;
         else if (param == string("IMG_WIDTH")) input >> image_width;
         else if (param == string("BLOCKSIZE")) input >> blocksize;
         else if (param == string("SADWINDOW")) input >> sadwindow;
         else if (param == string("TOTAL_CBCR")) input >> total_cbcr;
         else if (param == string("TOTAL_Y")) input >> total_y;
         else if (param == string("FRAMECOUNT")) input >> framecount;
         else if (param == string("UC_RATIO")) input >> uc_ratio;
         else if (param == string("FSWIN")) input >> fs_win;
         else if (param == string("BSWIN")) input >> bs_win;
         else if (param == string("REPLACE")) input >> enable_rep;
		 else if (param == string("FIRST_FS")) input >> first_fs;
		 else if (param == string("NEW3DRS")) input >> enable_new3DRS;
		 else if (param == string("EARLY_TERMINATE")) input >> enable_early_t;
		 else if (param == string("ALL_FS")) input >> all_fs;
		 else if (param == string("BI_FS")) input >> bi_fs;
		 else if (param == string("UPDATE")) input >> enable_update;
		 else if (param == string("OCCLUSION_TH")) input >> occlusion_th;
		 else if (param == string("PASS_COUNT")) input >> passcount;
		 else if (param == string("INPUT_VIDEO")) input >> input_video;
		 else if (param == string("OUTPUT_VIDEO")) input >> output_video;
		 else if (param == string("MVs_FILE_BI")) input >> Bi_MVs_filename;
		 else if (param == string("MVs_FILE")) input >> MVs_filename;
		 else if (param == string("MV_X_FILE")) input >> MV_X_file;
		 else if (param == string("MV_Y_FILE")) input >> MV_Y_file;
		 else if (param == string("RESULTS")) input >> results_file;
		 else if (param == string("CANDIDATE_COUNT")) input >> candidate_count;
		 else if (param == string("EXT_CANDIDATE_COUNT")) input >> ext_candidate_count;
		 else if (param == string("VECTOR_THRESHOLD")) input >> vector_threshold;
		 else if (param == string("SAD_THRESHOLD")) input >> SAD_threshold;
		 else if (param == string("UC_ALGO"))
		 {
			 string temp;
			 input >> temp;
			 if (temp == string("FAVG")) 
				 up_conversion_algo = 1;
			 else if (temp == string("DYNM"))
				 up_conversion_algo = 2;
			 else if (temp == string("TWOM"))
				 up_conversion_algo = 3;
			 else if (temp == string("BI_FAVG"))
				 up_conversion_algo = 4;
			 else if (temp == string("OBMC"))
				 up_conversion_algo = 5;
			 else if (temp == string("MC"))
				 up_conversion_algo = 6;
			 else if (temp == string("STAM"))
				 up_conversion_algo = 7;
		 }
		 
		 else if (param == string("SEARCH_LOCS"))
		 {
			 for (int i=0;i<candidate_count*2;i++)
			 {
				 input >> search_location_list[i];
			 }
		 }
		 else if (param == string("EXT_SEARCH_LOCS"))
		 {
			 for (int i=0;i<ext_candidate_count*2;i++)
			 {
				 input >> ext_search_location_list[i];
			 }
		 }
		 else if (param == string("SAME_SPATIAL_COEF")) input >> weights[0];
		 else if (param == string("SAME_TEMPORAL_COEF")) input >> weights[1];
		 else if (param == string("SAME_UPDATE_COEF")) input >> weights[2];
		 else if (param == string("SPATIAL_COEF")) input >> spatial_weight;
		 else if (param == string("TEMPORAL_COEF")) input >> temporal_weight;
		 else if (param == string("UPDATE_COEF")) input >> update_weight;

         
      }
   }
