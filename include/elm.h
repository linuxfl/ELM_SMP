#ifndef _ELM_H__
#define _ELM_H__
/*classification and regression*/
#define CLASSIFICATION_TRAINING 1
#define REGRESSION_TRAINING 0
#define ELM_TYPE CLASSIFICATION_TRAINING

/*number of neurons on every layer*/
#define NUMROWS 6
#define INPUT_NEURONS (NUMROWS-1)
#define HIDDEN_NEURONS 1500
#define OUTPUT_NEURONS 1
#define LUMMA 5
/*the size of data set*/
#define DATASET 380000
/*elm train*/

float RandomFloat(float start,float end);
void ELMTrain();
void RandomWeight(float **weight,int row,int column);
void RandomWeight_s(float *weight,int row,int column);
void RandomBiase(float *biase,int row);
void SigmoidHandle(float **matrix,int row,int column);
void SigmoidHandle_s(float *matrix,int row,int column);
#endif
