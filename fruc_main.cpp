/****************************************************************************/ 
/*			Motion-Compensated Frame-Rate Up-Conversion						*/
/****************************************************************************/
/*																			*/
/*Features:																	*/
/*Motion Estimation Methods:												*/
/*		3DRS																*/
/*		An Adaptive True Motion Estimation (ATME)							*/
/*		Bi-lateral Search (with initial vector selection)					*/
/*		Full-Search															*/
/*Up-Conversion Methods:													*/
/*	-Motion Compensated Field Averaging										*/
/*	-Motion Compensated Field Averaging for Bilateral Search				*/
/*	-Dynamic Median Filtering												*/
/*  -Static Median Filtering												*/
/*	-Two Mode Interpolation with Occlusion Detection using the Vector Field	*/
/*	-OBMC																	*/
/*	-Motion Compensation for Video Coding									*/
/*																			*/
/*Option to up-convert input video by a conversion factor of 2				*/
/*or re-generate and overwrite the even numbered frames for testing			*/
/*																			*/
/*	Coder: Mert CETIN (mertc at sabanciuniv dot edu)						*/
/*																			*/
/*	Sabanci University														*/
/*	Faculty of Engineering and Natural Sciences								*/
/*	System on Chip Design and Testing Group									*/
/*	http://fens.sabanciuniv.edu/soclab/										*/
/*																			*/
/*	All Rights Reserved														*/
/*	Please Do Not Use Without This Header									*/
/*																			*/
/****************************************************************************/


#include "config.h"
#include <iostream>
#include <string>
#include "randgen.h"
#include <math.h>
#include <algorithm>
#include "fruc_def.h"
#include "rand31-park-miller-carta-int.c"
using namespace std;


/*--------------------------------------------------------------------------*/
/* program entry point														*/
/* opens input and output files sets relevant file pointers					*/
/* reads input video file data into 2D arrays inside a loop and				*/
/* calls relevant motion estimation and up-conversion functions accordingly	*/
/* writes image data into output files										*/
/* runs comparison procedure and outputs the results						*/
/*--------------------------------------------------------------------------*/

int main()
{
	Config::reload("config.txt");


	FILE *infile;
	FILE *MVs_File_out;
	FILE *SAD_File_out;
	FILE *bi_MVs_File_out;
	FILE *outvid;
	FILE *MV_X;
	FILE *MV_Y;

	

	mvectors = (vector*) malloc((Config::image_height/Config::blocksize)*(Config::image_width/Config::blocksize)*sizeof vector);
	interMVs = (vector*) malloc((Config::image_height/Config::blocksize)*(Config::image_width/Config::blocksize)*sizeof vector);
	SAD_Values = new unsigned int[(Config::image_height/Config::blocksize)*(Config::image_width/Config::blocksize)] ;


	if((infile = fopen(Config::input_video.c_str(), "rb")) == NULL) {
		printf("Error : Input file could not be opened\n");
		return 1;
	}

	if((MVs_File_out = fopen(Config::MVs_filename.c_str(), "w")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}

	if((SAD_File_out = fopen("SADs.txt", "w")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}

	if((MV_X = fopen(Config::MV_X_file.c_str(), "w")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}

	if((MV_Y = fopen(Config::MV_Y_file.c_str(), "w")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}

	if((bi_MVs_File_out = fopen(Config::Bi_MVs_filename.c_str(), "w")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}

	if((outvid = fopen(Config::output_video.c_str(), "wb")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}

	if((vectortest = fopen("vectortest.txt", "w")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}



	int i,j;

	unsigned char* data1 = new unsigned char[Config::image_height*Config::image_width];

	unsigned char* data2 = new unsigned char[Config::image_height*Config::image_width];

	unsigned char *ResizedFrame1, *ResizedFrame2;

	rand31pmc_seedi(1);						// seed LFSR with 1

	int s_count = Config::candidate_count;
	int ext_s_count = Config::ext_candidate_count;
	int up_conv_algo = Config::up_conversion_algo;

	vector* s_locs;
	s_locs = new vector[s_count];
	for(i=0;i<s_count;i++){
		s_locs[i].x = Config::search_location_list[2*i];
		s_locs[i].y = Config::search_location_list[2*i+1];
	}

	vector* ext_s_locs;
	ext_s_locs = new vector[ext_s_count];
	for(i=0;i<ext_s_count;i++){
		ext_s_locs[i].x = Config::ext_search_location_list[2*i];
		ext_s_locs[i].y = Config::ext_search_location_list[2*i+1];
	}

	/*int s_count;

	fscanf(variables,"%d",&s_count);

	vector* s_locs;
	s_locs = new vector[s_count];

	for(i=0;i<s_count;i++){
	fscanf(variables,"%d",&(s_locs[i].x));
	fscanf(variables,"%d",&(s_locs[i].y));
	}*/



	firstframe = true;
	secondframe = true;

	// write first frame to output directly
	for (i=0;i<Config::image_height;i++){
		fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile);
		fwrite(&data1[INDEX(i,0)],sizeof(unsigned char), Config::image_width, outvid);
	}


	ResizedFrame1 = ResizeFrame(data1);

	/*	DEBUG - write first resized frame to file		*/
	//for (i=0;i<(Config::image_height+2*Config::fs_win);i++){
	//	fwrite(&ResizedFrame1[i*(Config::image_width+2*Config::fs_win)],sizeof(unsigned char), (Config::image_width+2*Config::fs_win), resized_test_out);
	//}
	/*													*/

	int loopcount;

	if(Config::enable_rep)
		loopcount = Config::framecount/4;
	else
		loopcount = Config::framecount/2;

	if (Config::enable_rep)									// if replace even numbered frames with up-converted ones
	{
		fseek(infile,Config::total_y,SEEK_CUR);				// skip the second frame
	}

	printf("Interpolating:    ");


	for (j=0;j<loopcount;j++)
	{	
		printf("\b\b\b%03d",j*4);						// display current frame number

		// read the next frame
		for (int i=0;i<Config::image_height;i++){
			fread(&data2[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile);
		}

		// resize
		ResizedFrame2 = ResizeFrame(data2);

		// process motion estimation stage

		if (Config::bi_fs)
		{
			if (Config::first_fs && firstframe)
				frameFullSearch(ResizedFrame2,ResizedFrame1,MVs_File_out,MV_X,MV_Y);
			else
			{
				calcFrame3DRS(ResizedFrame2,ResizedFrame1,MVs_File_out,MV_X,MV_Y,s_count,ext_s_count,s_locs,ext_s_locs,SAD_File_out);
			}
			frameBiFullSearch(ResizedFrame2,ResizedFrame1,bi_MVs_File_out);
		}
		else if (Config::all_fs)
		{
			frameFullSearch(ResizedFrame2,ResizedFrame1,MVs_File_out,MV_X,MV_Y);	
		}
		else if(Config::first_fs && firstframe)
			frameFullSearch(ResizedFrame2,ResizedFrame1,MVs_File_out,MV_X,MV_Y);
		else
		{
			firstpass = true;
			for (i=0;i<Config::passcount;i++){

				calcFrame3DRS(ResizedFrame2,ResizedFrame1,MVs_File_out,MV_X,MV_Y,s_count,ext_s_count,s_locs,ext_s_locs,SAD_File_out);
				firstpass = false;
			}

		}

		// process frame interpolation
		if(up_conv_algo == 1)
		{
			mc_field_average(ResizedFrame2,ResizedFrame1,outvid);
		}
		else if(up_conv_algo == 2)
		{
			DynMedian(ResizedFrame2,ResizedFrame1,outvid);
		}
		else if (up_conv_algo == 3)
		{
			two_mode_interpolate(ResizedFrame2,ResizedFrame1,outvid);
		}
		else if (up_conv_algo == 4)
		{
			bi_mc_field_average(ResizedFrame2,ResizedFrame1,outvid);
		}
		else if (up_conv_algo == 5)
		{
			obmc(ResizedFrame2,ResizedFrame1,outvid);
		}
		else if (up_conv_algo == 6)
		{
			motion_compensate(ResizedFrame2,ResizedFrame1,outvid);
		}
		else if(up_conv_algo == 7)
		{
			StaMedian(ResizedFrame2,ResizedFrame1,outvid);
		}


		// write next frame to output directly (odd numbered frame)
		if(up_conv_algo != 6)
		{
			for (i=0;i<Config::image_height;i++){
				fwrite(&data2[INDEX(i,0)],sizeof(unsigned char), Config::image_width, outvid);
			}
		}

		// if last frame
		if (j == loopcount-1){
			if(up_conv_algo != 6)
			{
				for (i=0;i<Config::image_height;i++){
					fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile);
					fwrite(&data1[INDEX(i,0)],sizeof(unsigned char), Config::image_width, outvid);
				}
			}

			break;
		}

		// if not, exchange the image data pointers and continue the process
		else {
			if (Config::enable_rep)									// if replace even numbered frames with up-converted ones
			{
				fseek(infile,Config::total_y,SEEK_CUR);				// skip the second frame
			}
			for (i=0;i<Config::image_height;i++){
				fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile);
			}

			ResizedFrame1 = ResizeFrame(data1);

			if (Config::all_fs)
			{
				frameFullSearch(ResizedFrame1,ResizedFrame2,MVs_File_out,MV_X,MV_Y);
			} 
			else if (Config::bi_fs)
			{
				calcFrame3DRS(ResizedFrame1,ResizedFrame2,MVs_File_out,MV_X,MV_Y,s_count,ext_s_count,s_locs,ext_s_locs,SAD_File_out);
				frameBiFullSearch(ResizedFrame1,ResizedFrame2,bi_MVs_File_out);					
			}
			else
			{
				firstpass = true;
				for (i=0;i<Config::passcount;i++){
					calcFrame3DRS(ResizedFrame1,ResizedFrame2,MVs_File_out,MV_X,MV_Y,s_count,ext_s_count,s_locs,ext_s_locs,SAD_File_out);
					firstpass = false;
				}

			}				

			if(up_conv_algo == 1)
			{
				mc_field_average(ResizedFrame1,ResizedFrame2,outvid);
			}
			else if(up_conv_algo == 2)
			{
				DynMedian(ResizedFrame1,ResizedFrame2,outvid);
			}
			else if (up_conv_algo == 3)
			{
				two_mode_interpolate(ResizedFrame1,ResizedFrame2,outvid);
			}
			else if (up_conv_algo == 4)
			{
				bi_mc_field_average(ResizedFrame1,ResizedFrame2,outvid);
			}
			else if (up_conv_algo == 5)
			{
				obmc(ResizedFrame1,ResizedFrame2,outvid);
			}
			else if (up_conv_algo == 6)
			{
				motion_compensate(ResizedFrame1,ResizedFrame2,outvid);
			}
			else if(up_conv_algo == 7)
			{
				StaMedian(ResizedFrame1,ResizedFrame2,outvid);
			}


			if(up_conv_algo != 6)
			{
				for (i=0;i<Config::image_height;i++){
					fwrite(&data1[INDEX(i,0)],sizeof(unsigned char), Config::image_width, outvid);
				}
			}
			if (Config::enable_rep)									// if replace even numbered frames with up-converted ones
			{
				fseek(infile,Config::total_y,SEEK_CUR);				// skip the second frame
			}
		}
	}

	// close file pointers
	fclose(infile);
	fclose(MVs_File_out);
	fclose(SAD_File_out);
	fclose(MV_X);
	fclose(MV_Y);
	fclose(bi_MVs_File_out);
	fclose(outvid);
	fclose(vectortest);
	


	/************************************************************************/
	/*				Comparison Test and PSNR Output                         */
	/************************************************************************/
	FILE *results;
	//FILE *resultsSAD;
	FILE *infile1;
	FILE *infile2;

	if((results = fopen(Config::results_file.c_str(), "a")) == NULL) {
		printf("Error : Output file could not be opened\n");
		return 1;
	}
	//if((resultsSAD = fopen("SAD.txt", "w")) == NULL) {
	//	printf("Error : Output file could not be opened\n");
	//	return 1;
	//}

	if((infile1 = fopen(Config::input_video.c_str(), "rb")) == NULL) {
		printf("Error : Input file 1 could not be opened\n");
		return 1;
	}

	if((infile2 = fopen(Config::output_video.c_str(), "rb")) == NULL) {
		printf("Error : Input file 2 could not be opened\n");
		return 1;
	}

	int f,framesgenerated;
	double SqError,MSE,TotMSE;
	TotMSE = 0.0;
	framesgenerated = 0;

	printf("\nComparing:    ");
	for (f=0;f<Config::framecount;f++){
	
		SqError = 0.0;
		MSE = 0.0;


		printf("\b\b\b%03d",f);
		for (i=0;i<Config::image_height;i++){
			fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile1);
			fread(&data2[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile2);
		}
	/*	for (i=0;i<Config::image_height;i++){
			if(Config::up_conversion_algo==6 && Config::enable_rep == 1)
			{
				if(f == 0 || f == Config::framecount/2)
				{
					fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile1);
				}
				else 
				{
					fseek(infile1,Config::total_y,SEEK_CUR);
					fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile1);
				}
					
			}
			else
			fread(&data1[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile1);

			fread(&data2[INDEX(i,0)], sizeof(unsigned char), Config::image_width, infile2);
		}*/

		for (i=0;i<Config::image_height;i++){
			for (j=0;j<Config::image_width;j++){
				SqError = SqError + pow(double(data1[INDEX(i,j)] - data2[INDEX(i,j)]),2);
			}
		}

		MSE = SqError / (Config::image_height*Config::image_width);
		if (MSE != 0) framesgenerated++;

		TotMSE += MSE;
	
		/*if( Config::up_conversion_algo==6 && Config::enable_rep == 1 && f == (Config::framecount / 2))
			break;*/
	}
	//printf("\n");


	if(TotMSE == 0)
		fprintf (results,"PSNR: %s\t\tSAD Count: %d\n",Config::enable_rep?"infinity":"N/A",SADcount);
	else
	{
		if(Config::enable_rep || up_conv_algo == 6)
		{
			fprintf(results,"PSNR: %3.4f\t\tSAD Count: %d\n",10.0*log10((255.0*255.0)/(TotMSE/framesgenerated)),SADcount);
			/*fprintf(results,"%3.4f",10.0*log10((255.0*255.0)/(TotMSE/framesgenerated)));
			fprintf(resultsSAD,"%d",SADcount);*/
			//			fprintf(results,"Total PSNR: %3.4f\t\t Generated Frames Only PSNR: %3.4f\t\t%d\n",10.0*log10((255.0*255.0)/(TotMSE/f)),10.0*log10((255.0*255.0)/(TotMSE/framesgenerated)),SADcount);
		}
		else
			fprintf(results,"PSNR: N/A\t\tSAD Count: %d\n",SADcount);
	}



	delete [] data1 ;
	delete [] data2 ;
	delete [] ResizedFrame1;
	delete [] ResizedFrame2;

	free(interMVs);
	free(mvectors);

	fclose(infile1);
	fclose(infile2);
	fclose(results);
	//fclose(resultsSAD);

	return(0);
}


/*------------------------------------------------------------------------------*/
/*Dynamic Median Filter caller function											*/
/*Calls Dynamic Median Filter calculator function for every pixel in the frame	*/
/*and writes generated image data into output video file						*/
/*------------------------------------------------------------------------------*/
void DynMedian(unsigned char* curData, unsigned char* refData, FILE* outvid){	

	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];
	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			if (i<1072)
			{
				tempframe[INDEX(i,j)] = DynMedian_Pixel(curData,refData,i,j);
			}
			else
			{
				tempframe[INDEX(i,j)] = (refData[R_INDEX(i,j)]+curData[R_INDEX(i,j)])/2;				
			}
		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}

/*------------------------------------------------------------------------------*/
/*Static Median Filter caller function											*/
/*Calls Static Median Filter calculator function for every pixel in the frame	*/
/*and writes generated image data into output video file						*/
/*------------------------------------------------------------------------------*/
void StaMedian(unsigned char* curData, unsigned char* refData, FILE* outvid){	

	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];
	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			if (i<1072)
			{
				tempframe[INDEX(i,j)] = StaMedian_Pixel(curData,refData,i,j);
			}
			else
			{
				tempframe[INDEX(i,j)] = (refData[R_INDEX(i,j)]+curData[R_INDEX(i,j)])/2;				
			}

		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}


/*------------------------------------------------------------------------------------------------------*/
/*Dynamic Median Filter calculator function																*/
/*calculate two motion compensated pixel data one from previous one from current frames					*/
/*and median them with non-motion compensated interpolated pixel data from previous and current frames	*/
/*------------------------------------------------------------------------------------------------------*/
unsigned char DynMedian_Pixel(unsigned char* curData, unsigned char* refData, int rowpxpos, int colpxpos){
	unsigned char result[3];
	vector avg_vector;
	avg_vector=mvectors[((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize)];
	result[0]= refData[R_INDEX((rowpxpos-(avg_vector.y/2)),(colpxpos-(avg_vector.x/2)))];
	result[1]= curData[R_INDEX((rowpxpos+(avg_vector.y/2)),(colpxpos+(avg_vector.x/2)))];
	result[2]= (refData[R_INDEX(rowpxpos,colpxpos)] + curData[R_INDEX(rowpxpos,colpxpos)])/2;

	if (result[0] > result[1]){
		if (result[0] > result[2]){
			if (result[1] > result[2]) return result[1];
			else return result[2];
		}
		else return result[0];
	}
	else{
		if (result[0] < result[2]){
			if (result[1] < result[2]) return result[1];
			else return result[2];
		}
		else return result[0];
	}
}

/*------------------------------------------------------------------------------------------------------*/
/*Static Median Filter calculator function																*/
/*calculate two non-motion compensated pixel data one from previous one from current frames				*/
/*and median them with motion compensated pixel data from previous and current frames					*/
/*------------------------------------------------------------------------------------------------------*/
unsigned char StaMedian_Pixel(unsigned char* curData, unsigned char* refData, int rowpxpos, int colpxpos){
	unsigned char result[3];
	vector avg_vector;
	avg_vector=mvectors[((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize)];
	//result[0]= refData[R_INDEX((rowpxpos-(avg_vector.y/2)),(colpxpos-(avg_vector.x/2)))];
	//result[1]= curData[R_INDEX((rowpxpos+(avg_vector.y/2)),(colpxpos+(avg_vector.x/2)))];
	//result[2]= (refData[R_INDEX(rowpxpos,colpxpos)] + curData[R_INDEX(rowpxpos,colpxpos)])/2;

	result[0]= refData[R_INDEX(rowpxpos,colpxpos)];
	result[1]= curData[R_INDEX(rowpxpos,colpxpos)];
	result[2]= ((refData[R_INDEX((rowpxpos-(avg_vector.y/2)),(colpxpos-(avg_vector.x/2)))])+curData[R_INDEX((rowpxpos+(avg_vector.y/2)),(colpxpos+(avg_vector.x/2)))])/2;

	if (result[0] > result[1]){
		if (result[0] > result[2]){
			if (result[1] > result[2]) return result[1];
			else return result[2];
		}
		else return result[0];
	}
	else{
		if (result[0] < result[2]){
			if (result[1] < result[2]) return result[1];
			else return result[2];
		}
		else return result[0];
	}
}


/*------------------------------------------------------------------------------------------------------*/
/*Motion Compensated Field Averaging calculator function												*/
/*calculate two motion compensated pixel data one from previous one from current frames					*/
/*and interpolate them	by 50%																			*/
/*------------------------------------------------------------------------------------------------------*/
unsigned char mc_field_average_pixel(unsigned char* curData, unsigned char* refData, int rowpxpos, int colpxpos){
	unsigned char field_average;
	vector avg_vector;
	avg_vector=mvectors[((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize)];

	if(rowpxpos<1072)
	{
		field_average = ((refData[R_INDEX((rowpxpos-(avg_vector.y/2)),(colpxpos-(avg_vector.x/2)))])+curData[R_INDEX((rowpxpos+(avg_vector.y/2)),(colpxpos+(avg_vector.x/2)))])/2;
	}
	else
		field_average = (refData[R_INDEX(rowpxpos,colpxpos)]+curData[R_INDEX(rowpxpos,colpxpos)])/2;

	return field_average;
}

/*------------------------------------------------------------------------------*/
/*Motion Compensation for Video Coding caller function							*/
/*Calls Motion Compensation calculator function for every pixel in the frame	*/
/*and writes generated image data into output video file						*/
/*------------------------------------------------------------------------------*/
void motion_compensate(unsigned char* curData, unsigned char* refData, FILE* outvid){
	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];

	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			if (i<1072)
			{
				tempframe[INDEX(i,j)] = motion_compensate_pixel(curData,refData,i,j);
			}
			else
			{
				tempframe[INDEX(i,j)] = refData[R_INDEX(i,j)];			
			}

		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}

/*------------------------------------------------------------------------------------------------------*/
/*Motion Compensation for Video Coding calculator function												*/
/*use the MV data for the current frame to interpolate using only the previous frame's image data		*/
/*------------------------------------------------------------------------------------------------------*/
unsigned char motion_compensate_pixel(unsigned char* curData, unsigned char* refData, int rowpxpos, int colpxpos){
	unsigned char compensated;
	vector avg_vector;
	avg_vector=mvectors[((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize)];

	compensated = refData[R_INDEX((rowpxpos-(avg_vector.y)),(colpxpos-(avg_vector.x)))];

	return compensated;
}

/*------------------------------------------------------------------------------------------------------*/
/*Motion Compensated Field Averaging for Bilateral ME calculator function								*/
/*calculate two motion compensated pixel data one from previous one from current frames					*/
/*and interpolate them	by 50%																			*/
/*this function should only be used when MVs returned from bilateral ME function are not multiplied by 2*/
/*------------------------------------------------------------------------------------------------------*/
unsigned char bi_mc_field_average_pixel(unsigned char* curData, unsigned char* refData, int rowpxpos, int colpxpos){
	unsigned char field_average;
	vector avg_vector;
	avg_vector=interMVs[((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize)];
	//avg_vector=mvectors[((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize)];
	field_average = ((refData[R_INDEX((rowpxpos-(avg_vector.y)),(colpxpos-(avg_vector.x)))])+curData[R_INDEX((rowpxpos+(avg_vector.y)),(colpxpos+(avg_vector.x)))])/2;
	return field_average;
}

/*------------------------------------------------------------------------------*/
/*Motion Compensated Field Averaging caller function							*/
/*Calls MC Field Averaging calculator function for every pixel in the frame		*/
/*and writes generated image data into output video file						*/
/*------------------------------------------------------------------------------*/
void mc_field_average(unsigned char* curData, unsigned char* refData, FILE* outvid){
	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];

	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			tempframe[INDEX(i,j)] = mc_field_average_pixel(curData,refData,i,j);
		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}

/***************************************************************/
/* MC Field Averaging Function for Bilateral Motion Estimation */
/* !!USE ONLY IF MOTION VECTORS ARE NOT MULTIPLIED BY 2        */
/*           INSIDE THE BILATERAL SEARCH FUNCTIONS!!!!!!!!!!!!!*/
/***************************************************************/
void bi_mc_field_average(unsigned char* curData, unsigned char* refData, FILE* outvid){

	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];
	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			if (i<1072)
			{
				tempframe[INDEX(i,j)] = bi_mc_field_average_pixel(curData,refData,i,j);
			}
			else
			{
				tempframe[INDEX(i,j)] = (refData[R_INDEX(i,j)]+curData[R_INDEX(i,j)])/2;				
			}

		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}

/*------------------------------------------------------------------------------*/
/*Two Mode Interpolator decision function										*/
/*For every pixel in the image frame run occlusion detection algorithm	 		*/
/*if it's decided that there is occlusion situation use dynamic median filtering*/
/*if there's no occlusion then use MC Field averaging							*/
/*------------------------------------------------------------------------------*/
void two_mode_interpolate(unsigned char* curData, unsigned char* refData, FILE* outvid){
	//unsigned char tempframe[Config::image_height][Config::image_width];

	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];
	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			if (i<1072)
			{
				if (detect_occlusion((i/Config::blocksize),(j/Config::blocksize)))
					tempframe[INDEX(i,j)] = DynMedian_Pixel(curData,refData,i,j);
				else
					tempframe[INDEX(i,j)] = mc_field_average_pixel(curData,refData,i,j);
			}
			else
			{
				tempframe[INDEX(i,j)] = (refData[R_INDEX(i,j)]+curData[R_INDEX(i,j)])/2;				
			}

		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}

/*--------------------------------------------------------------------------------------*/
/*Occlusion detection function															*/
/*If the previous block's motion vector is larger than the next block's motion vector	*/
/*more than a pre-defined threshold then it is decided that there is occlusion			*/
/*--------------------------------------------------------------------------------------*/
bool detect_occlusion(int rowpos, int colpos){
	if (rowpos == 0) rowpos += 1;
	else if (rowpos == BLOCK_COUNT_OF_COLUMN) rowpos -= 1;
	else if (colpos == 0) colpos += 1;
	else if (colpos == BLOCK_COUNT_OF_ROW) colpos -= 1;

	if(abs(mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos+1].x - mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos-1].x) > Config::occlusion_th || 
		abs(mvectors[(rowpos+1)*(BLOCK_COUNT_OF_ROW)+colpos].y - mvectors[(rowpos-1)*(BLOCK_COUNT_OF_ROW)+colpos].y) > Config::occlusion_th)
		return true;
	else
		return false;
}

/*------------------------------------------------------*/
/* Overloaded Updater Function version 1				*/
/* if input is a vector pointer than update data where	*/
/* it points with a random vector						*/
/*------------------------------------------------------*/
vector* update(vector* mvector){
	if(Config::enable_update){
		vector randvector;
		long unsigned int randomint;
		randomint = rand31pmc_ranlui()%25;
		//RandGen rand;
		//randvector = updateSet[rand.RandInt(0,24)];
		randvector = updateSet[randomint];
		mvector->x += randvector.x;
		mvector->y += randvector.y;
	}
	return mvector;
}

/*------------------------------------------------------*/
/* Overloaded Updater Function version 2				*/
/* if input is a vector than update that vector's data	*/
/* with a random vector									*/
/*------------------------------------------------------*/
vector update(vector mvector){
	if(Config::enable_update){
		vector randvector;
		//RandGen rand;
		long unsigned int randomint;
		randomint = rand31pmc_ranlui()%25;
		//randvector = updateSet[rand.RandInt(0,24)];
		randvector = updateSet[randomint];
		mvector.x += randvector.x;
		mvector.y += randvector.y;
	}
	return mvector;
}


/*----------------------------------------------------------------------------------------------*/
/*3DRS block matcher function	(Two updated candidates)										*/
/*----------------------------------------------------------------------------------------------*/
vector calcBlock3DRS(unsigned char* curData, unsigned char* refData, int rowpos, int colpos,int s_count, vector* s_locs){

	vector mvector = {0,0};
	vector* tempvector = new vector[s_count+1];

	unsigned int* SAD;
	SAD = new unsigned int[s_count];
	int i;
	int row,col;
	unsigned int min = Config::blocksize*Config::blocksize*255;



	if (Config::enable_early_t && calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)]) == 0){
		mvector.x = 0;
		mvector.y = 0;
	}
	else if (firstframe){
		update(&mvector);		
	}

	else{
		for(i=0;i<s_count;i++){ //use for(i=0;i<=s_count;i++) to include zero motion vector

			row = rowpos + s_locs[i].y;
			col = colpos + s_locs[i].x;
			if (row < 0) row = 0;
			else if (row > (BLOCK_COUNT_OF_COLUMN - 1)) row = BLOCK_COUNT_OF_COLUMN -1;
			if (col < 0) col = 0;
			else if (col > (BLOCK_COUNT_OF_ROW - 1)) col = BLOCK_COUNT_OF_ROW -1;

			if (i == s_count-1)
			{
				tempvector[i] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]);
			}
			else if (i == s_count-2)
			{
				//tempvector[i].x = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].x;
				//tempvector[i].y = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].y;
				tempvector[i] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]); //comment out this line and uncomment the above 2 lines for single updated candidate
			}
			else if (i == s_count)
			{
				tempvector[i].x = 0;
				tempvector[i].y = 0;
			}
			else
			{	
				tempvector[i].x = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].x;
				tempvector[i].y = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].y;

			}


			/*if (!firstpass && !secondframe && (tempvector[i].x == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].x && tempvector[i].y == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].y))
			{
			SAD[i] = SAD_Values[rowpos*(BLOCK_COUNT_OF_ROW)+colpos];
			} 
			else
			{*/
			SAD[i] = calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX((rowpos*Config::blocksize-tempvector[i].y),colpos*Config::blocksize-tempvector[i].x)]);
			//}

		}
		for(i=0;i<s_count;i++){ //use for(i=0;i<=s_count;i++) to include zero motion vector
			if (SAD[i] < min){
				mvector = tempvector[i];
				min = SAD[i];
			}		
		}


	}
	min_SAD = min;
	delete [] SAD;
	delete [] tempvector;
	return mvector;
}

/********************************************************************************************/
/*ATME Algorithm calculator function														*/
/********************************************************************************************/
vector calcNewBlock3DRS(unsigned char* curData, unsigned char* refData, int rowpos, int colpos,int s_count,int ext_s_count, vector* s_locs, vector* ext_s_locs){

	vector mvector = {0,0};
	vector* tempvector = new vector[ext_s_count+1];
	//vector* tempvector = new vector[s_count+1];
	vector updatevector;

	unsigned int* SAD;
	SAD = new unsigned int[ext_s_count+1];
	int i;
	int row,col;
	unsigned int min = Config::blocksize*Config::blocksize*255;
	bool under_threshold = false;
	float* weight = new float[ext_s_count+1];
	vector* candidates = new vector[2];
	vector vmedian;
	vector* sortarray = new vector[3];
	vector sorttemp;

	if (Config::enable_early_t && calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)]) == 0){
		mvector.x = 0;
		mvector.y = 0;
	}
	else if (firstframe){
		update(&mvector);		
	}

	else{

		for(i=0;i<s_count;i++){

			row = rowpos + s_locs[i].y;
			col = colpos + s_locs[i].x;
			if (row < 0) row = 0;
			else if (row > (BLOCK_COUNT_OF_COLUMN - 1)) row = BLOCK_COUNT_OF_COLUMN -1;
			if (col < 0) col = 0;
			else if (col > (BLOCK_COUNT_OF_ROW - 1)) col = BLOCK_COUNT_OF_ROW -1;

			// uncomment below lines for giving weights to candidates
			/*if (i == s_count-1)
			{
			tempvector[i] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]);
			weight[i] = Config::update_weight;
			}
			else if (i == s_count-2)
			{
			weight[i] = Config::temporal_weight;
			tempvector[i].x = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].x;
			tempvector[i].y = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].y;
			}
			else
			{	
			weight[i] = Config::spatial_weight;
			tempvector[i].x = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].x;
			tempvector[i].y = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].y;

			}*/
			if (i == s_count-1)
			{
				tempvector[i] = mvectors[row*(BLOCK_COUNT_OF_ROW)+col];
				//tempvector[i] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]); //uncomment this line for secondary update vector added candidate
				//updatevector = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]);

			}
			else
			{
				tempvector[i].x = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].x;
				tempvector[i].y = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].y;
			}




		}

		
		//calculate median
		vmedian = L1_MedianFilter(tempvector,s_count);

		fprintf(vectortest,"%d\t%d\t(%d %d)\t%d\t(%d %d)\t%d\t(%d %d)\t%d\t->\t(%d %d)\t",rowpos,colpos,tempvector[0].x,tempvector[0].y,L1_Norm(tempvector[0],tempvector[1]),tempvector[1].x,tempvector[1].y,L1_Norm(tempvector[1],tempvector[2]),tempvector[2].x,tempvector[2].y,L1_Norm(tempvector[0],tempvector[2]),vmedian.x,vmedian.y);
		
		// under Vth check
		for (int a = 0 ; a <s_count ; a++)
		{
			for (int b = a; b < s_count ; b++)
			{
				if (L1_Norm(tempvector[a],tempvector[b]) > Config::vector_threshold)
				{
					under_threshold = false;
					break;
				}
				else
				{
					under_threshold = true;
					
				}
			}
			if (under_threshold == false)
			{
				break;
			}
			
		}
		
		if(under_threshold)
			{

			

			candidates[0] = vmedian;
			
			candidates[1] = update(candidates[0]);
			
			fprintf(vectortest,"(%d %d)\t",candidates[1].x,candidates[1].y); 
			//candidates[1] = update(mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos]);
			for(i=0;i<2;i++){
				if (!firstpass && !secondframe && (candidates[i].x == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].x && candidates[i].y == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].y))
				{
					SAD[i] = SAD_Values[rowpos*(BLOCK_COUNT_OF_ROW)+colpos];
				} 
				else
				{
					SAD[i] = calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX((rowpos*Config::blocksize-candidates[i].y),colpos*Config::blocksize-candidates[i].x)]);
				}
			}
			for(i=0;i<2;i++){
				if (SAD[i] < min){
					mvector = candidates[i];
					min = SAD[i];
				}		
			}
			fprintf(vectortest,"(%d %d)\t1\n",mvector.x,mvector.y); 
			
		}
		else
		{

			//tempvector[s_count-1] = updatevector;
			tempvector[s_count-1] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]);
			//tempvector[s_count-2] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]); // uncomment this line to add another update vector
			
			//tempvector[s_count].x = 0; //uncomment these lines to include zero motion vector
			//tempvector[s_count].y = 0; //..

			fprintf(vectortest,"(%d %d)\t",tempvector[s_count-1].x,tempvector[s_count-1].y);
			

			for(i=0;i<s_count;i++){ //use for(i=0;i<=s_count;i++) to include zero motion vector

				if (!firstpass && !secondframe && (tempvector[i].x == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].x && tempvector[i].y == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].y))
				{
					SAD[i] = SAD_Values[rowpos*(BLOCK_COUNT_OF_ROW)+colpos];
				} 
				else
				{
					SAD[i] = calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX((rowpos*Config::blocksize-tempvector[i].y),colpos*Config::blocksize-tempvector[i].x)]);
				}
			}
			for(i=0;i<s_count;i++){ //use for(i=0;i<=s_count;i++) to include zero motion vector
				if (SAD[i] < min){
					mvector = tempvector[i];
					min = SAD[i];
				}		
			}
			fprintf(vectortest,"(%d %d)\t2\n",mvector.x,mvector.y); 

			if (min > Config::SAD_threshold)
			{
				
				for(i=0;i<=ext_s_count;i++){

					row = rowpos + ext_s_locs[i].y;
					col = colpos + ext_s_locs[i].x;
					if (row < 0) row = 0;
					else if (row > (BLOCK_COUNT_OF_COLUMN - 1)) row = BLOCK_COUNT_OF_COLUMN -1;
					if (col < 0) col = 0;
					else if (col > (BLOCK_COUNT_OF_ROW - 1)) col = BLOCK_COUNT_OF_ROW -1;


					if (i == ext_s_count)
					{
						tempvector[i].x = 0;
						tempvector[i].y = 0;
					}
					else if (i == ext_s_count-1)
					{
						tempvector[i] = update(mvectors[row*(BLOCK_COUNT_OF_ROW)+col]);

					}
					else
					{
						tempvector[i].x = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].x;
						tempvector[i].y = mvectors[row*(BLOCK_COUNT_OF_ROW)+col].y;
					}
				}

				for(i=0;i<=ext_s_count;i++){

					if (!firstpass && !secondframe && (tempvector[i].x == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].x && tempvector[i].y == mvectors[rowpos*(BLOCK_COUNT_OF_ROW)+colpos].y))
					{
						SAD[i] = SAD_Values[rowpos*(BLOCK_COUNT_OF_ROW)+colpos];
					} 
					else
					{
						SAD[i] = calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX((rowpos*Config::blocksize-tempvector[i].y),colpos*Config::blocksize-tempvector[i].x)]);
					}
				}
				for(i=0;i<=ext_s_count;i++){
					if (SAD[i] < min){
						mvector = tempvector[i];
						min = SAD[i];
					}		
				}


			} 

		}


	}
	min_SAD = min;
	delete [] tempvector;
	delete [] candidates;
	delete [] SAD;
	delete [] weight;
	return mvector;
}

/*----------------------------------------------------------------------------------*/
/*3DRS caller function																*/
/*Calls 3DRS block matcher function for every block in the frame					*/
/*and writes the motion vectors into motion vectors array and also into a file		*/
/*----------------------------------------------------------------------------------*/
void calcFrame3DRS(unsigned char* curData, unsigned char* refData,FILE* outfile,FILE* MV_X,FILE* MV_Y,int s_count,int ext_s_count, vector* s_locs, vector* ext_s_locs,FILE* SAD_File){

	int i,j;

	for (i=0;(i<(BLOCK_COUNT_OF_COLUMN) && i < 67);i++){
		for (j=0;j<(BLOCK_COUNT_OF_ROW);j++){
			if (Config::enable_new3DRS)
			{
				mvectors[(i*(BLOCK_COUNT_OF_ROW))+j] = calcNewBlock3DRS(curData,refData,i,j,s_count,ext_s_count,s_locs,ext_s_locs);
				SAD_Values[(i*(BLOCK_COUNT_OF_ROW))+j] = min_SAD;
			}
			else
			{
				mvectors[(i*(BLOCK_COUNT_OF_ROW))+j] = calcBlock3DRS(curData,refData,i,j,s_count,s_locs);
				SAD_Values[(i*(BLOCK_COUNT_OF_ROW))+j] = min_SAD;
			}

			fprintf(outfile,"(%3d,%3d) ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].x,mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].y);
			fprintf(MV_X,"%d ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].x);
			fprintf(MV_Y,"%d ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].y);
			fprintf(SAD_File,"%5d ",SAD_Values[(i*BLOCK_COUNT_OF_ROW)+j]);
		}
		fprintf(outfile,"\n");
		fprintf(MV_X,"\n");
		fprintf(MV_Y,"\n");
		fprintf(SAD_File,"\n");
	}
	if (firstframe)
	{
		firstframe = false;
	}
	else if (secondframe)
	{
		secondframe = false;
	}
	fprintf(outfile,"\n\n");
	fprintf(MV_X,"\n\n");
	fprintf(MV_Y,"\n\n");
	fprintf(SAD_File,"\n\n");
	//fprintf(vectortest,"\n");



	return;
}

/*----------------------------------------------------------------------------------*/
/*Full Search caller function														*/
/*calls fullsearch calculator function for every block in the frame					*/
/*and writes the motion vectors into motion vectors array and also into a file		*/
/*----------------------------------------------------------------------------------*/
void frameFullSearch(unsigned char* curData, unsigned char* refData,FILE* outfile,FILE* MV_X,FILE* MV_Y){

	int i,j;
	for (i=0;i<(BLOCK_COUNT_OF_COLUMN);i++){
		for (j=0;j<(BLOCK_COUNT_OF_ROW);j++){
			mvectors[(i*(BLOCK_COUNT_OF_ROW))+j] = blockFullSearch(curData,refData,i,j);
			fprintf(outfile,"(%3d,%3d) ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].x,mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].y);
			fprintf(MV_X,"%d ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].x);
			fprintf(MV_Y,"%d ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].y);
		}
		fprintf(outfile,"\n");
		fprintf(MV_X,"\n");
		fprintf(MV_Y,"\n");
	}
	if (firstframe) firstframe =0;
	fprintf(outfile,"\n\n");
	fprintf(MV_X,"\n\n");
	fprintf(MV_Y,"\n\n");

	return;
}

/*--------------------------------------------------------------------------------------*/
/*Full Search calculator function														*/
/*takes previous and current frames' image data and current block's position as inputs	*/
/*first compares the current block with the block in the same position at prev. frame	*/
/*then calculates the search window range and limits that range inside the boundaries	*/
/*and finally calculates the SAD for all of the block pixels and output the best vector	*/
/*--------------------------------------------------------------------------------------*/
vector blockFullSearch(unsigned char* curData, unsigned char* refData, int rowpos, int colpos){
	int i,j,rowSrcPosLow,rowSrcPosHigh,colSrcPosLow,colSrcPosHigh,SADmin,SADtemp;
	vector mvector={0,0};

	if (Config::enable_early_t && calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)]) == 0){
		mvector.x = 0;
		mvector.y = 0;
	}
	else{
		rowSrcPosLow = rowpos*Config::blocksize-Config::fs_win;
		rowSrcPosHigh = rowpos*Config::blocksize+Config::fs_win;
		colSrcPosLow = colpos*Config::blocksize-Config::fs_win;
		colSrcPosHigh = colpos*Config::blocksize+Config::fs_win;


		// out of bounds check - no need to use with resized frame

		/*if (rowSrcPosLow < 0)
		rowSrcPosLow = 0;
		if (rowSrcPosHigh > Config::image_height - Config::blocksize)
		rowSrcPosHigh = Config::image_height - Config::blocksize;
		if (colSrcPosLow < 0)
		colSrcPosLow = 0;
		if (colSrcPosHigh < Config::image_width - Config::blocksize)
		colSrcPosHigh = Config::image_width - Config::blocksize;*/

		SADmin = 255*Config::blocksize*Config::blocksize;

		for(i=rowSrcPosLow;i<rowSrcPosHigh;i++){
			for(j=colSrcPosLow;j<colSrcPosHigh;j++){
				SADtemp = calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX(i,j)]);
				if (SADtemp < SADmin){
					SADmin = SADtemp;
					mvector.x = colpos*Config::blocksize-(j);
					mvector.y = rowpos*Config::blocksize-(i);
				}
			}
		}
	}

	return mvector;

}

/**********************************************************/
/*		Bilateral Search Caller Function				  */
/*********************************************************/
void frameBiFullSearch(unsigned char* curData, unsigned char* refData,FILE* bioutfile)
{

	int i,j;
	vector initalMV = {0,0};


	for (i=0;i<(BLOCK_COUNT_OF_COLUMN);i++){
		for (j=0;j<(BLOCK_COUNT_OF_ROW);j++){
			//interMVs[(i*(BLOCK_COUNT_OF_ROW))+j] = blockBilateral(curData,refData,i,j,mvectors[i*(BLOCK_COUNT_OF_ROW)+j]);
			mvectors[(i*(BLOCK_COUNT_OF_ROW))+j] = blockBilateral(curData,refData,i,j,mvectors[i*(BLOCK_COUNT_OF_ROW)+j]);
			fprintf(bioutfile,"(%3d,%3d) ",mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].x,mvectors[(i*(BLOCK_COUNT_OF_ROW))+j].y);

		}
		fprintf(bioutfile,"\n");
	}
	if (firstframe) firstframe =0;
	fprintf(bioutfile,"\n");
	fprintf(bioutfile,"\n");

	return;
}

/**************************************/
//Bilateral Search Calculator function//
/*************************************/
vector blockBilateral(unsigned char* curData, unsigned char* refData, int rowpos, int colpos, vector initialMV)
{
	int i,j,SADmin,SADtemp;
	vector mvector={0,0};
	int BSWINX, BSWINY;
	vector ActualInitialMV = {initialMV.x/2,initialMV.y/2}; 
	vector AbsoluteInitVector = {abs(initialMV.x)/2,abs(initialMV.y)/2};

	if (Config::enable_early_t && calculateSAD(&curData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)],&refData[R_INDEX(rowpos*Config::blocksize,colpos*Config::blocksize)]) == 0){
		mvector.x = 0;
		mvector.y = 0;
	}
	else{
		if (( colpos*Config::blocksize - AbsoluteInitVector.x  - Config::bs_win ) < 0)
			BSWINX = colpos*Config::blocksize - AbsoluteInitVector.x;
		else
			BSWINX = Config::bs_win;

		if (( colpos*Config::blocksize + AbsoluteInitVector.x + Config::bs_win ) > (Config::image_width - Config::blocksize))
			BSWINX = (Config::image_width - Config::blocksize) - (colpos*Config::blocksize + AbsoluteInitVector.x);

		if (( rowpos*Config::blocksize - AbsoluteInitVector.y - Config::bs_win) < 0) 
			BSWINY = rowpos*Config::blocksize -AbsoluteInitVector.y;
		else
			BSWINY = Config::bs_win;
		if (( rowpos*Config::blocksize + AbsoluteInitVector.y + Config::bs_win ) > (Config::image_height - Config::blocksize))
			BSWINY = (Config::image_height - Config::blocksize) - (rowpos*Config::blocksize+AbsoluteInitVector.y);

		if (BSWINX < 0 || BSWINY<0)
			cout << "HOP!"<<endl;

		SADmin = 255*Config::blocksize*Config::blocksize;

		//for (i = -BSWINY ; i <= BSWINY ; i++)
		//{
		//	for ( j = -BSWINX; j<= BSWINX ; j++)
		//	{
		//		SADtemp = calculateSAD(&curData[R_INDEX((rowpos*Config::blocksize+ActualInitialMV.y+i),(colpos*Config::blocksize +ActualInitialMV.x + j))],&refData[R_INDEX(((rowpos*Config::blocksize-ActualInitialMV.y-i)),(colpos*Config::blocksize-ActualInitialMV.x -j))]);
		//		if (SADtemp < SADmin)
		//		{
		//			SADmin = SADtemp;
		//			//mvector.x = (ActualInitialMV.x+j);
		//			//mvector.y = (ActualInitialMV.y+i);
		//			mvector.x = (ActualInitialMV.x+j)*2;
		//			mvector.y = (ActualInitialMV.y+i)*2;
		//		}
		//	}
		//}

		int x = 0;
		int y = 0;
		int dx = 0;
		int dy = -1;
		int temp;
		int loopX = BSWINX*2+1;
		int loopY = BSWINY*2+1;
		for (i = 0; i < max(loopX,loopY)*max(loopX,loopY) ; i++)
		{
			if ((-loopX/2 <= x) && (x <= loopX/2) && (-loopY/2 <= y) && (y <= loopY/2))
			{
				//cout << x << " " << y << endl;
				SADtemp = calculateSAD(&curData[R_INDEX((rowpos*Config::blocksize+ActualInitialMV.y+y),(colpos*Config::blocksize +ActualInitialMV.x + x))],&refData[R_INDEX(((rowpos*Config::blocksize-ActualInitialMV.y-y)),(colpos*Config::blocksize-ActualInitialMV.x -x))]);
				if (SADtemp < SADmin)
				{
					SADmin = SADtemp;
					//mvector.x = (ActualInitialMV.x+j);
					//mvector.y = (ActualInitialMV.y+i);
					mvector.x = (ActualInitialMV.x+x)*2;
					mvector.y = (ActualInitialMV.y+y)*2;
				}
			}
			if ( (x == y) || (x < 0 && x == -y) || (x > 0 && x == 1-y) )
			{
				temp = dx;
				dx = -dy;
				dy = temp;
			}
			x += dx;
			y += dy;

		}


	}

	return mvector;

}

/*************************************************************************************/
/*		Resize passed frame by fullsearch window size from all sides				 */
/*		This is done by taking the mirror image of the blocks at sides and copying	 */
/*		them outside of the default image, eventually enlarging frame data			 */
/*************************************************************************************/
unsigned char* ResizeFrame (unsigned char* orgininalFrame)
{
	int ResizedWidth = (Config::image_width+Config::fs_win*2);
	int ResizedHeight = (Config::image_height+Config::fs_win*2);

	unsigned char* ResizedFrame = new unsigned char[ResizedHeight*ResizedWidth];
	int i,j;
	int FetchPosition_Row;
	int FetchPosition_Column;
	for (i=0;i<ResizedHeight;i++){
		for (j=0;j<ResizedWidth;j++){
			if (i<Config::fs_win)
			{
				FetchPosition_Row = (Config::fs_win-1-i);
			} 
			else if (i>=Config::fs_win+Config::image_height)
			{
				FetchPosition_Row = (2*Config::image_height+Config::fs_win-1-i);
			}
			else
			{
				FetchPosition_Row = i - Config::fs_win;
			}
			if (j<Config::fs_win)
			{
				FetchPosition_Column = (Config::fs_win-1-j);
			}
			else if(j>= Config::image_width+Config::fs_win)
			{
				FetchPosition_Column = (2*Config::image_width+Config::fs_win-1-j);
			}
			else
			{
				FetchPosition_Column = j-Config::fs_win;
			}
			ResizedFrame[i*ResizedWidth+j] = orgininalFrame[INDEX(FetchPosition_Row,FetchPosition_Column)];
		}

	}

	return ResizedFrame;

}




/********************************************************/
/* Overlapped Block Motion Compensation Caller Function */
/********************************************************/
void obmc(unsigned char* curData, unsigned char* refData, FILE* outvid){

	unsigned char* tempframe = new unsigned char[Config::image_width*Config::image_height];
	int i,j;
	for (i=0;i<Config::image_height;i++){
		for (j=0;j<Config::image_width;j++){
			tempframe[INDEX(i,j)] = mc_obmc_pixel(curData,refData,i,j);
		}
	}
	for (i=0;i<Config::image_height;i++){
		fwrite(&tempframe[INDEX(i,0)],sizeof(unsigned char), Config::image_width,outvid);
	}
	delete [] tempframe;
}

/************************************************************/
/* Overlapped Block Motion Compensation Calculator Function */
/************************************************************/
unsigned char mc_obmc_pixel(unsigned char* curData, unsigned char* refData, int rowpxpos, int colpxpos){
	unsigned char field_average;
	unsigned char field_average1,field_average2,field_average3,field_average4;
	int BLOCK=((rowpxpos/Config::blocksize)*(BLOCK_COUNT_OF_ROW))+(colpxpos/Config::blocksize);

	vector avg_vector;
	vector vector1,vector2,vector3,vector4;
	if(!((colpxpos>Config::blocksize)&&(colpxpos<(Config::image_width-Config::blocksize))&&(rowpxpos>Config::blocksize)&&(rowpxpos<(Config::image_height-Config::blocksize))))
	{
		avg_vector=mvectors[BLOCK];
		field_average = ((refData[R_INDEX((rowpxpos-(avg_vector.y/2)),(colpxpos-(avg_vector.x/2)))])+curData[R_INDEX((rowpxpos+(avg_vector.y/2)),(colpxpos+(avg_vector.x/2)))])/2;
	}
	else if( (colpxpos%Config::blocksize<=(Config::blocksize/4)) && (rowpxpos%Config::blocksize<=(Config::blocksize/4)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK-1];
		vector3=mvectors[BLOCK-(BLOCK_COUNT_OF_ROW)];
		vector4=mvectors[BLOCK-(BLOCK_COUNT_OF_ROW)-1];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average3 = ((refData[R_INDEX((rowpxpos-(vector3.y/2)),(colpxpos-(vector3.x/2)))])+curData[R_INDEX((rowpxpos+(vector3.y/2)),(colpxpos+(vector3.x/2)))])/2;
		field_average4 = ((refData[R_INDEX((rowpxpos-(vector4.y/2)),(colpxpos-(vector4.x/2)))])+curData[R_INDEX((rowpxpos+(vector4.y/2)),(colpxpos+(vector4.x/2)))])/2;
		field_average= (field_average1+field_average2+field_average3+field_average4)/4;
	}
	else if((colpxpos%Config::blocksize>(Config::blocksize-Config::blocksize/4) || (colpxpos%Config::blocksize==0)) && (rowpxpos%Config::blocksize<=(Config::blocksize/4)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK+1];
		vector3=mvectors[BLOCK-(BLOCK_COUNT_OF_ROW)];
		vector4=mvectors[BLOCK-(BLOCK_COUNT_OF_ROW)+1];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average3 = ((refData[R_INDEX((rowpxpos-(vector3.y/2)),(colpxpos-(vector3.x/2)))])+curData[R_INDEX((rowpxpos+(vector3.y/2)),(colpxpos+(vector3.x/2)))])/2;
		field_average4 = ((refData[R_INDEX((rowpxpos-(vector4.y/2)),(colpxpos-(vector4.x/2)))])+curData[R_INDEX((rowpxpos+(vector4.y/2)),(colpxpos+(vector4.x/2)))])/2;
		field_average= (field_average1+field_average2+field_average3+field_average4)/4;
	}
	else if((colpxpos%Config::blocksize<=(Config::blocksize/4)) && (rowpxpos%Config::blocksize>(Config::blocksize-Config::blocksize/4) || (rowpxpos%Config::blocksize==0)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK-1];
		vector3=mvectors[BLOCK+(BLOCK_COUNT_OF_ROW)];
		vector4=mvectors[BLOCK+(BLOCK_COUNT_OF_ROW)-1];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average3 = ((refData[R_INDEX((rowpxpos-(vector3.y/2)),(colpxpos-(vector3.x/2)))])+curData[R_INDEX((rowpxpos+(vector3.y/2)),(colpxpos+(vector3.x/2)))])/2;
		field_average4 = ((refData[R_INDEX((rowpxpos-(vector4.y/2)),(colpxpos-(vector4.x/2)))])+curData[R_INDEX((rowpxpos+(vector4.y/2)),(colpxpos+(vector4.x/2)))])/2;
		field_average= (field_average1+field_average2+field_average3+field_average4)/4;
	}
	else if((colpxpos%Config::blocksize>(Config::blocksize-Config::blocksize/4)|| (colpxpos%Config::blocksize==0)) && (rowpxpos%Config::blocksize>(Config::blocksize-Config::blocksize/4) || (rowpxpos%Config::blocksize==0)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK+1];
		vector3=mvectors[BLOCK+(BLOCK_COUNT_OF_ROW)];
		vector4=mvectors[BLOCK+(BLOCK_COUNT_OF_ROW)+1];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average3 = ((refData[R_INDEX((rowpxpos-(vector3.y/2)),(colpxpos-(vector3.x/2)))])+curData[R_INDEX((rowpxpos+(vector3.y/2)),(colpxpos+(vector3.x/2)))])/2;
		field_average4 = ((refData[R_INDEX((rowpxpos-(vector4.y/2)),(colpxpos-(vector4.x/2)))])+curData[R_INDEX((rowpxpos+(vector4.y/2)),(colpxpos+(vector4.x/2)))])/2;
		field_average= (field_average1+field_average2+field_average3+field_average4)/4;
	}
	else if((colpxpos%Config::blocksize>(Config::blocksize/4)) && (colpxpos%Config::blocksize<=(Config::blocksize-Config::blocksize/4)) && (rowpxpos%Config::blocksize<=(Config::blocksize/4)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK-BLOCK_COUNT_OF_ROW];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average= (field_average1+field_average2)/2;
	}
	else if((colpxpos%Config::blocksize>(Config::blocksize/4)) && (colpxpos%Config::blocksize<=(Config::blocksize-Config::blocksize/4)) && (rowpxpos%Config::blocksize>(Config::blocksize-Config::blocksize/4) || (rowpxpos%Config::blocksize==0)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK+BLOCK_COUNT_OF_ROW];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average= (field_average1+field_average2)/2;
	}
	else if((colpxpos%Config::blocksize<=(Config::blocksize/4)) && (rowpxpos%Config::blocksize<=(Config::blocksize-Config::blocksize/4)) && (rowpxpos%Config::blocksize>(Config::blocksize/4)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK-1];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average= (field_average1+field_average2)/2;
	}
	else if((colpxpos%Config::blocksize>(Config::blocksize-Config::blocksize/4) || (colpxpos%Config::blocksize==0)) && (rowpxpos%Config::blocksize<=(Config::blocksize-Config::blocksize/4)) && (rowpxpos%Config::blocksize>(Config::blocksize/4)))
	{
		vector1=mvectors[BLOCK];
		vector2=mvectors[BLOCK+1];
		field_average1 = ((refData[R_INDEX((rowpxpos-(vector1.y/2)),(colpxpos-(vector1.x/2)))])+curData[R_INDEX((rowpxpos+(vector1.y/2)),(colpxpos+(vector1.x/2)))])/2;
		field_average2 = ((refData[R_INDEX((rowpxpos-(vector2.y/2)),(colpxpos-(vector2.x/2)))])+curData[R_INDEX((rowpxpos+(vector2.y/2)),(colpxpos+(vector2.x/2)))])/2;
		field_average= (field_average1+field_average2)/2;
	}
	else
	{
		avg_vector=mvectors[BLOCK];
		field_average = ((refData[R_INDEX((rowpxpos-(avg_vector.y/2)),(colpxpos-(avg_vector.x/2)))])+curData[R_INDEX((rowpxpos+(avg_vector.y/2)),(colpxpos+(avg_vector.x/2)))])/2;
	}


	return field_average;
}

/* Function to calculate pixel-wise L1 Norm pf 2 MVs */
int L1_Norm(vector V1,vector V2)
{
	return abs(V1.x - V2.x) + abs(V1.y - V2.y);
}

/* Function to find the median vector among a set a motion vectors */
vector L1_MedianFilter(vector* VectorSet,int VectorCount)
{
	int temp_median;
	int min_median = 9999;
	int Which_Vector = 0;

	for (int i = 0; i< VectorCount;i++)
	{
		temp_median = 0;
		for (int j = 0; j<VectorCount;j++)
		{
			temp_median +=   L1_Norm(VectorSet[i],VectorSet[j]);
		}
		if( temp_median < min_median)
		{
			Which_Vector = i;
			min_median = temp_median;
		}
	}

	return VectorSet[Which_Vector];
}

/* Function that outputs progression of pixel locations during spiral search*/
void spiral(int X, int Y)
{
	int x = 0;
	int y = 0;
	int dx = 0;
	int dy = -1;
	int temp;
	int i;
	for (i = 0; i < max(X,Y)*max(X,Y) ; i++)
	{
		if ((-X/2 <= x) && (x <= X/2) && (-Y/2 <= y) && (y <= Y/2))
			cout << x << " " << y << endl;
		if ( (x == y) || (x < 0 && x == -y) || (x > 0 && x == 1-y) )
		{
			temp = dx;
			dx = -dy;
			dy = temp;
		}
		x += dx;
		y += dy;

	}
}
