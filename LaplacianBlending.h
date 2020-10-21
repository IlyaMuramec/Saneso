#pragma once

#include "opencv2/opencv.hpp"
#include "ProcTimes.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcv.h"

using namespace cv;

#define FLOAT_BLEND

class LaplacianBlending {
public:
	LaplacianBlending(ProcTimes *_procTimes, int _levels);
	~LaplacianBlending();

	void blend(const Mat& _left, const Mat& _right, const Mat& _blendMask, Mat& _output);

private:

	void buildLaplacianPyramid(const Mat& img, Mat &lap, Mat& smallestLevel);
	void reconstructImgFromLapPyramid(Mat &results);

	vector<Mat_<Vec3f> > maskGaussianPyramid;
	Mat leftLap, rightLap, blendedLap, blendedResults;
    Mat leftSmallestLevel, rightSmallestLevel, resultSmallestLevel;
    int levels; 
	ProcTimes *procTimes;

	IppiSize pyrRoi;
	IppiPyramid *gPyr; // pointer to Gaussian down pyramid structure 
	IppiPyramid *gUpPyr; // pointer to Gaussian Up pyramid structure
	Ipp32f **gImage;
	Ipp32f **gUpImage;

	Ipp8u *pBufferDown;
	Ipp8u* pBufferUp;

	void clearIPPBuffers();
	void createIPPBuffers(IppiSize srcRoi);
     
	void buildPyramids();
	void buildGaussianPyramid(const Mat& blendMask);
	void blendLapPyrs();
	void combineWithIPP(const Mat& left, const Mat& right, const Mat& mask, Mat& results);
};
