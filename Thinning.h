#pragma once

/**
* Code for thinning a binary image using Zhang-Suen algorithm.
*/
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

class CThinning
{
public:
	CThinning();
	~CThinning();

	static void CThinning::thinningIteration(cv::Mat& im, int iter);
	static void CThinning::thinning(cv::Mat& im);
};