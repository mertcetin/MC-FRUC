#ifndef _FRUC_DEF_H
#define _FRUC_DEF_H


#include <iostream>

#include "config.h"
using namespace std;


#define R_INDEX(X,Y) ((X + Config::fs_win)*(Config::image_width+2*Config::fs_win) + Y + Config::fs_win)
#define SAD_INDEX(X,Y) (X *(Config::image_width+2*Config::fs_win) + Y )
#define INDEX(X, Y) ((X * Config::image_width) + Y)

#define BLOCK_COUNT_OF_ROW (Config::image_width/Config::blocksize)
#define BLOCK_COUNT_OF_COLUMN (Config::image_height/Config::blocksize)


typedef struct vector	// motion vector struct with two data elements
{
	int x;
	int y;
} vector;

bool firstframe;
bool secondframe;

bool firstpass;

unsigned double SADcount= 0;
vector zerovector= {0,0};

unsigned int min_SAD;

FILE* vectortest;


vector* mvectors; //= new vector[(BLOCK_COUNT_OF_COLUMN)*(BLOCK_COUNT_OF_ROW)];

unsigned int* SAD_Values;

vector* interMVs; //= new vector[(BLOCK_COUNT_OF_COLUMN)*(BLOCK_COUNT_OF_ROW)];

vector updateSet[] = {	{-3,-2},{-3,-1},{-3,0},{-3,1},{-3,2},	// update set
						{-1,-2},{-1,-1},{-1,0},{-1,1},{-1,2},	// for x axis (-3,-1,0,1,3)
						{0,-2},{0,-1},{0,0},{0,1},{0,2},		// for y axis (-2,-1,0,1,2)
						{1,-2},{1,-1},{1,0},{1,1},{1,2},		// assumed that motion is higher likely
						{3,-2},{3,-1},{3,0},{3,1},{3,2}		};	// in the x direction


// SAD calculation function
unsigned int calculateSAD(unsigned char* curBlock, unsigned char* previousBlock){

	int SADresult = 0;
	int i,j;

	SADcount++;
	
	for (i=0;i<Config::blocksize;i++){
		for (j=0;j<Config::blocksize;j++){
			SADresult += abs(curBlock[SAD_INDEX(i,j)] - previousBlock[SAD_INDEX(i,j)]);
		}
	}

	return SADresult;
}




vector* update(vector*);
vector update(vector);
vector update();
vector calcBlock3DRS(unsigned char*, unsigned char*, int, int);
vector calcNewBlock3DRS(unsigned char*, unsigned char*, int, int, int, int, vector*, vector*);

void calcFrame3DRS(unsigned char*, unsigned char*,FILE*,FILE*,FILE*, int,int, vector*,vector*,FILE*);
void frameFullSearch(unsigned char*, unsigned char*,FILE*,FILE*,FILE*);
vector blockFullSearch(unsigned char*, unsigned char*, int, int);
unsigned char mc_field_average_pixel(unsigned char*, unsigned char*, int, int);
void mc_field_average(unsigned char*, unsigned char*, FILE*);
unsigned char motion_compensate_pixel(unsigned char*, unsigned char*, int, int);
void motion_compensate(unsigned char*, unsigned char*, FILE*);
unsigned char bi_mc_field_average_pixel(unsigned char*, unsigned char*, int, int);
void bi_mc_field_average(unsigned char*, unsigned char*, FILE*);
void DynMedian(unsigned char*, unsigned char*, FILE*);
void StaMedian(unsigned char*, unsigned char*, FILE*);
unsigned char DynMedian_Pixel(unsigned char*, unsigned char*, int, int);
unsigned char StaMedian_Pixel(unsigned char*, unsigned char*, int, int);
bool detect_occlusion(int, int);
void two_mode_interpolate(unsigned char*, unsigned char*, FILE*);
vector blockBilateral(unsigned char*, unsigned char*, int, int, vector);
void frameBiFullSearch(unsigned char*, unsigned char*,FILE*);
unsigned char* ResizeFrame (unsigned char*);
void obmc(unsigned char*, unsigned char*, FILE*);
unsigned char mc_obmc_pixel(unsigned char*, unsigned char*, int, int);
int L1_Norm(vector ,vector );
vector L1_MedianFilter(vector*,int);
void Spiral(int,int);
#endif