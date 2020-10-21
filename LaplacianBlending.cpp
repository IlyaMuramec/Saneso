#include "StdAfx.h"
#include "LaplacianBlending.h"
#include "TimingData.h"

#define USE_IPP
//#undef USE_IPP

LaplacianBlending::LaplacianBlending(ProcTimes *_procTimes, int _levels) :
procTimes(_procTimes),
levels(_levels) 
{
	pyrRoi.width = 0;
	pyrRoi.height = 0;
}

LaplacianBlending::~LaplacianBlending()
{
	clearIPPBuffers();
}

void LaplacianBlending::clearIPPBuffers()
{
	if ((pyrRoi.width > 0 || pyrRoi.height > 0))
	{
		IppiPyramidDownState_32f_C3R **gState = (IppiPyramidDownState_32f_C3R**)&(gPyr->pState);
		IppiPyramidUpState_32f_C3R **gUpState = (IppiPyramidUpState_32f_C3R**)&(gUpPyr->pState);

		ippiPyramidLayerDownFree_32f_C3R(*gState);
		ippiPyramidLayerUpFree_32f_C3R(*gUpState);

		// free allocated images
		for (int i = 1; i <= levels; i++)
		{
			ippiFree(gImage[i]);
			ippiFree(gUpImage[i - 1]);
		}

		// free pyramid structures
		ippiPyramidFree(gPyr);
		ippiPyramidFree(gUpPyr);

		delete[] pBufferDown;
		delete[] pBufferUp;
	}
}

void LaplacianBlending::createIPPBuffers(IppiSize srcRoi)
{
	// allocate pyramid structures
	Ipp32f rate = 2;
	const int kerSize = 3;
	Ipp32f pKernel[3] = { 1.f, 1.f, 1.f };
	ippiPyramidInitAlloc(&gPyr, levels, srcRoi, rate);
	ippiPyramidInitAlloc(&gUpPyr, levels, srcRoi, rate);

	IppiPyramidDownState_32f_C3R **gState = (IppiPyramidDownState_32f_C3R**)&(gPyr->pState);
	IppiPyramidUpState_32f_C3R **gUpState = (IppiPyramidUpState_32f_C3R**)&(gUpPyr->pState);

	// allocate structures to calculate pyramid layers
	int mode = IPPI_INTER_LINEAR;
	ippiPyramidLayerDownInitAlloc_32f_C3R(gState, srcRoi, rate, pKernel, kerSize, mode);
	ippiPyramidLayerUpInitAlloc_32f_C3R(gUpState, srcRoi, rate, pKernel, kerSize, mode);

	gImage = (Ipp32f**)(gPyr->pImage);
	gUpImage = (Ipp32f**)(gUpPyr->pImage);

	IppiSize *pRoi = gPyr->pRoi;
	int *gStep = gPyr->pStep;
	int *gUpStep = gUpPyr->pStep;
	for (int i = 0; i <= levels; i++)
	{
		gImage[i] = ippiMalloc_32f_C3(pRoi[i].width, pRoi[i].height, gStep + i);
		gUpImage[i] = ippiMalloc_32f_C3(pRoi[i].width, pRoi[i].height, gUpStep + i);
	}

	int bufferSize;
	ippiPyrDownGetBufSize_Gauss5x5(srcRoi.width, ipp32f, 3, &bufferSize);
	pBufferDown = new Ipp8u[bufferSize];
	ippiPyrUpGetBufSize_Gauss5x5(srcRoi.width, ipp32f, 3, &bufferSize);
	pBufferUp = new Ipp8u[bufferSize];

	pyrRoi.height = srcRoi.height;
	pyrRoi.width = srcRoi.width;

	leftLap = Mat(pRoi[0].height, pRoi[0].width, CV_32FC3);
	rightLap = Mat(pRoi[0].height, pRoi[0].width, CV_32FC3);
	blendedLap = Mat(pRoi[0].height, pRoi[0].width, CV_32FC3);
	blendedResults = Mat(pRoi[0].height, pRoi[0].width, CV_32FC3);
	leftSmallestLevel = Mat(pRoi[1].height, pRoi[1].width, CV_32FC3);
	rightSmallestLevel = Mat(pRoi[1].height, pRoi[1].width, CV_32FC3);
	resultSmallestLevel = Mat(pRoi[1].height, pRoi[1].width, CV_32FC3);
}

void LaplacianBlending::blend(const Mat& _left, const Mat& _right, const Mat& _blendMask, Mat& _output) {

	Mat_<float> m;
		
	TimingData timingData;

	IppiSize srcRoi = { _blendMask.cols, _blendMask.rows };
	if ((pyrRoi.width != srcRoi.width) || (pyrRoi.height != srcRoi.height))
	{
		clearIPPBuffers();
		createIPPBuffers(srcRoi);

		_blendMask.convertTo(m, CV_32F, 1.0 / 255.0);
		buildGaussianPyramid(m);
	}


	if (procTimes)
	{
		procTimes->AddTime("Blending", "Gaussian_Mask", timingData.Elapsed_ms());
		timingData.Restart();
	}

	buildLaplacianPyramid(_left, leftLap, leftSmallestLevel);

	if (procTimes)
	{
		procTimes->AddTime("Blending", "Laplacian_Left", timingData.Elapsed_ms());
		timingData.Restart();
	}

	buildLaplacianPyramid(_right, rightLap, rightSmallestLevel);

	if (procTimes)
	{
		procTimes->AddTime("Blending", "Laplacian_Right", timingData.Elapsed_ms());
		timingData.Restart();
	}

	blendLapPyrs();

	if (procTimes)
	{
		procTimes->AddTime("Blending", "Pyramids", timingData.Elapsed_ms());
		timingData.Restart();
	}

	reconstructImgFromLapPyramid(blendedResults);
	blendedResults.convertTo(_output, CV_8U);

	if (procTimes)
	{
		procTimes->AddTime("Blending", "Reconstruct", timingData.Elapsed_ms());
		timingData.Restart();
	}
}

void LaplacianBlending::buildGaussianPyramid(const Mat& blendMask) {
	maskGaussianPyramid.clear();
	Mat currentImg;
	cvtColor(blendMask, currentImg, CV_GRAY2BGR);
	maskGaussianPyramid.push_back(currentImg); //highest level

	Mat _down, _down3;
	pyrDown(blendMask, _down, leftSmallestLevel.size()); //smallest level
	cvtColor(_down, _down3, CV_GRAY2BGR);
	maskGaussianPyramid.push_back(_down3);
}

void LaplacianBlending::buildLaplacianPyramid(const Mat& imgByte, Mat& lap, Mat& smallestLevel)
{
	Mat_<Vec3f> img;
	imgByte.convertTo(img, CV_32F);

	//lapPyr.clear();

#ifdef USE_IPP
	IppiSize srcRoi = { img.cols, img.rows };

	IppiPyramidDownState_32f_C3R **gState = (IppiPyramidDownState_32f_C3R**)&(gPyr->pState);
	IppiPyramidUpState_32f_C3R **gUpState = (IppiPyramidUpState_32f_C3R**)&(gUpPyr->pState);

	IppiSize *pRoi = gPyr->pRoi;
	int *gStep = gPyr->pStep;
	int level = gPyr->level;
	int *gUpStep = gUpPyr->pStep;
	int srcStep = img.step;

	//// build Gaussian pyramid with level+1 layers
	//ippiCopy_32f_C3R((const float *)img.data, img.step, gImage[0], gStep[0], pRoi[0]);
	//for (int i = 1; i <= level; i++)
	//{
	//	ippiPyramidLayerDown_32f_C3R(gImage[i - 1], gStep[i - 1], pRoi[i - 1],
	//		gImage[i], gStep[i], pRoi[i], *gState);
	//}

	//// build Gaussian UP pyramid with level layers
	//for (int i = level - 1; i >= 0; i--)
	//{
	//	ippiPyramidLayerUp_32f_C3R(gImage[i + 1], gStep[i + 1], pRoi[i + 1], gUpImage[i], gUpStep[i], pRoi[i], *gUpState);

	//	Mat lap = Mat(pRoi[i].height, pRoi[i].width, CV_32FC3);
	//	ippiSub_32f_C3R((const float*)gUpImage[i], gUpStep[i], gImage[i], gStep[i], (float *)lap.data, lap.step, pRoi[i]);
	//	lapPyr.push_back(lap);
	//}

	//// Put the smallest in the results
	//smallestLevel = Mat(pRoi[1].height, pRoi[1].width, CV_32FC3);
	//ippiCopy_32f_C3R(gImage[levels], gStep[levels], (float *)smallestLevel.data, smallestLevel.step, pRoi[levels]);

	//smallestLevel = Mat(pRoi[1].height, pRoi[1].width, CV_32FC3);
	//Mat lap = Mat(pRoi[0].height, pRoi[0].width, CV_32FC3);
#if 1

	ippiCopy_32f_C3R((const float *)img.data, img.step, (float *)lap.data, lap.step, pRoi[0]);

	ippiPyrDown_Gauss5x5_32f_C3R((float *)lap.data, lap.step,
		(float*)smallestLevel.data, smallestLevel.step, pRoi[0], pBufferDown);
	
	ippiPyrUp_Gauss5x5_32f_C3R((const float*)smallestLevel.data, smallestLevel.step,
		(float *)lap.data, lap.step, pRoi[1], pBufferUp);

#else
	ippiPyramidLayerDown_32f_C3R((const float *)img.data, img.step, pRoi[0],
		(float*)smallestLevel.data, smallestLevel.step, pRoi[1], *gState);

	ippiPyramidLayerUp_32f_C3R((const float*)smallestLevel.data, smallestLevel.step, pRoi[1], 
		(float *)lap.data, lap.step, pRoi[0], *gUpState);
#endif

	ippiSub_32f_C3R((float *)lap.data, lap.step, (const float *)img.data, img.step, (float *)lap.data, lap.step, pRoi[0]);
	//lapPyr.push_back(lap);


#else
	Mat currentImg = img;
	for (int l = 0; l<levels; l++) {
		Mat down, up;
		pyrDown(currentImg, down);
		pyrUp(down, up, currentImg.size());
		Mat lap = currentImg - up;
		lapPyr.push_back(lap);
		currentImg = down;
	}
	currentImg.copyTo(smallestLevel);
#endif

}

void LaplacianBlending::reconstructImgFromLapPyramid(Mat & results)
{
	pyrUp(resultSmallestLevel, results, blendedLap.size());
	results += blendedLap;
}

void LaplacianBlending::combineWithIPP(const Mat& left, const Mat& right, const Mat& mask, Mat& results)
{
	Mat tempRow = Mat::zeros(1, mask.cols, CV_32FC3);

	for (int row = 0; row < mask.rows; row++)
	{
		uchar *pMask = &mask.data[row*mask.step];
		uchar *pAntiMask = tempRow.data;
		ippsSubCRev_32f((const float*)pMask, 1.0F, (float *)pAntiMask, 3 * mask.cols);

		uchar *pLeft = &left.data[row*left.step];
		uchar *pResults = &results.data[row*results.step];
		ippsMul_32f((const float*)pLeft, (const float*)pMask, (float *)pResults, 3 * mask.cols);

		uchar *pRight = &right.data[row*right.step];
		ippsMul_32f_I((const float*)pRight, (float *)pAntiMask, 3 * mask.cols);

		ippsAdd_32f_I((const float*)pAntiMask, (float *)pResults, 3 * mask.cols);
	}
}


void LaplacianBlending::blendLapPyrs() 
{
#ifdef USE_IPP
	combineWithIPP(leftSmallestLevel, rightSmallestLevel, maskGaussianPyramid.back(), resultSmallestLevel);
	combineWithIPP(leftLap, rightLap, maskGaussianPyramid[0], blendedLap);

#else
	resultSmallestLevel = leftSmallestLevel.mul(maskGaussianPyramid.back()) +
		rightSmallestLevel.mul(Scalar(1.0, 1.0, 1.0) - maskGaussianPyramid.back());

	resultLapPyr.clear();

	for (int l = 0; l<levels; l++) {
		Mat A = leftLapPyr[l].mul(maskGaussianPyramid[l]);
		Mat antiMask = Scalar(1.0, 1.0, 1.0) - maskGaussianPyramid[l];
		Mat B = rightLapPyr[l].mul(antiMask);
		Mat_<Vec3f> blendedLevel = A + B;

		resultLapPyr.push_back(blendedLevel);
	}
#endif
}