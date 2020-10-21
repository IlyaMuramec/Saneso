#include "StdAfx.h"
#include "MosaicData.h"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

#define MASK_SIDES 8

log4cxx::LoggerPtr  MosaicData::logger(log4cxx::Logger::getLogger("MosaicData"));

MosaicData::MosaicData(ProcTimes *pProcTimes) :
_laplacianBlending(pProcTimes, 1)
{
	_cameraMosaics.resize(5);

	//SetSize(Size(720, 720));
	SetSize(Size(1200, 1200));

	_ring30Radius = 0.5;
	_ring0Radius = 0.9;
	_crossHairRadius = _ring30Radius;
	_enableBlending = true;
	_blendWidth = 20;

	m_eImageStitchingMode = eNormal;
	m_bMarkerDisplayEnabled = false;
	m_bApplyMaskEnabled = false;

	_sideCameraScale = 0.35;
	_sideCameraRadius = 1.2;// 1.0;
	_sideCameraOffset = 0.3;

	_frontBorderAverageValues = vector<Scalar>(5);
}


MosaicData::~MosaicData(void)
{
}

void MosaicData::SetSize(cv::Size size)
{ 
	_size = size; 
	_tempMosiac = Mat::zeros(_size, CV_8UC3);

	_center = Point(_size.width / 2, _size.height / 2);

	for (int i = 0; i < 5; i++)
	{
		CameraMosaic *pCameraMosaic = &_cameraMosaics[i];
		pCameraMosaic->_image = Mat::zeros(_size, CV_8UC3);
		pCameraMosaic->_mask = Mat::zeros(_size, CV_8U);
	}
}

void MosaicData::SetCameraMosaic(CameraType camera, cv::Mat image, cv::Mat mask, cv::Rect rect, cv::Rect borderRect)
{
	CameraMosaic *pCameraMosaic = &_cameraMosaics[(int)camera];
	mask.copyTo(pCameraMosaic->_mask);
	image.copyTo(pCameraMosaic->_image, mask);
	pCameraMosaic->_boundingRect = rect;

	if (camera == eFront)
	{
		_borderRect = borderRect;
	}
}

void MosaicData::UpdateMosaic(CameraType camera, Mat &mosaic)
{
	try
	{
		CameraMosaic *pCameraMosaic = &_cameraMosaics[(int)camera];

		// Blend the results into the mosaic
		if ((camera == eFront) && IsBlendingEnabled() &&
			((m_eImageStitchingMode == eNormal) || (m_eImageStitchingMode == eClover)))
		{
			if (pCameraMosaic->_boundingRect.area() > 0)
			{
				Mat roiMosaic1 = mosaic(pCameraMosaic->_boundingRect);
				Mat roiMosaic2 = pCameraMosaic->_image(pCameraMosaic->_boundingRect);
				Mat roiMask = pCameraMosaic->_mask(pCameraMosaic->_boundingRect);

				_laplacianBlending.blend(roiMosaic2, roiMosaic1, roiMask, roiMosaic1);

				// Update the front camera average values
				_frontBorderAverageValues[eLeft] = GetBorderAverage(eLeft, pCameraMosaic->_image);
				_frontBorderAverageValues[eRight] = GetBorderAverage(eRight, pCameraMosaic->_image);
				_frontBorderAverageValues[eTop] = GetBorderAverage(eTop, pCameraMosaic->_image);
				_frontBorderAverageValues[eBottom] = GetBorderAverage(eBottom, pCameraMosaic->_image);
			}
		}
		else
		{
			pCameraMosaic->_image.copyTo(mosaic, pCameraMosaic->_mask);
		}
	}
	catch (cv::Exception ex)
	{
		LOG4CXX_ERROR(logger, "Error adding camera " << CameraNames[camera] << ": " << ex.msg);
	}
	
}

Scalar MosaicData::GetBorderAverage(CameraType side, Mat image)
{
	Scalar average;
	Rect rc;
	double sideFraction = 0.5;
	int width = 20;
	Point center = Point(_borderRect.x + _borderRect.width / 2, _borderRect.y + _borderRect.height / 2);

	switch (side)
	{
	case eLeft:
		rc = Rect(_borderRect.x, round(center.y - 0.5 * sideFraction * _borderRect.height), 
			width, sideFraction * _borderRect.height);
		average = mean(image(rc));
		break;
	case eRight:
		rc = Rect(_borderRect.x + _borderRect.width - width, round(center.y - 0.5 * sideFraction * _borderRect.height), 
			width, sideFraction * _borderRect.height);
		average = mean(image(rc));
		break;
	case eTop:
		rc = Rect(round(center.x - 0.5 * sideFraction * _borderRect.width), _borderRect.y, 
			sideFraction * _borderRect.width, width);
		average = mean(image(rc));
		break;
	case eBottom:
		rc = Rect(round(center.x - 0.5 * sideFraction * _borderRect.width), _borderRect.y + _borderRect.height - width,
			sideFraction * _borderRect.width, width);
		average = mean(image(rc));
		break;
	}

	// Make sure there isn't a zero in the mean, since it will cause a NAN
	for (int i = 0; i < 3; i++)
	{
		average[i] = max(1.0, average[i]);
	}
	
	return average;
}

void MosaicData::CreateMosaic(Mat &mosaic)
{
	_tempMosiac.setTo(0);
	UpdateMosaic(eTop, _tempMosiac);
	UpdateMosaic(eBottom, _tempMosiac);
	UpdateMosaic(eLeft, _tempMosiac);
	UpdateMosaic(eRight, _tempMosiac);
	UpdateMosaic(eFront, _tempMosiac);

	if (m_eImageStitchingMode == eNormal)
	{
		if (m_bApplyMaskEnabled)
		{
			ApplyMask(_tempMosiac);
		}

		// redraw the markers on top
		if (m_bMarkerDisplayEnabled)
		{
			DrawCrossHairsBlack(_tempMosiac);
			//DrawCrossHairs(_tempMosiac);
		}
	}

	if (!_saveFolder.empty())
	{
		cv::imwrite(_saveFolder + "\\MosiacFull.png", _tempMosiac);
	}

	if (mosaic.size() != _size)
	{
		mosaic = Mat::zeros(_size, CV_8UC3);
	}
	_tempMosiac.copyTo(mosaic);

	_framesPerSecond.NewFrame();
}

void MosaicData::DrawCrossHairs(Mat &mosaic)
{
	int halfWidth = mosaic.cols/2;
	int halfHeight = mosaic.rows/2;
	int minDim = min(halfWidth, halfHeight);
	double ring30Size = _ring30Radius * minDim;
	double ring0Size = _ring0Radius * minDim;
	Point center(halfWidth, halfHeight);
	double diff = (ring30Size + ring0Size) / 2 - ring30Size;

	circle(mosaic, Point(halfWidth, halfHeight), ring30Size - diff, Scalar(255, 255, 255), 1);
	circle(mosaic, Point(halfWidth,halfHeight), ring30Size, Scalar(255, 255, 255), 1);
	circle(mosaic, Point(halfWidth, halfHeight), ring30Size + diff, Scalar(255, 255, 255), 1);
	circle(mosaic, Point(halfWidth,halfHeight), ring0Size, Scalar(255, 255, 255), 1);
	line(mosaic, Point(0,halfHeight), Point(mosaic.cols,halfHeight), Scalar(255, 255, 255), 1);
	line(mosaic, Point(halfWidth,0), Point(halfWidth,mosaic.rows), Scalar(255, 255, 255), 1);

	// Draw rays
	for (double rad = 0.0; rad < 3.1415; rad += 3.1415/8.0)
	{
		double x = halfWidth * cos(rad);
		double y = halfWidth * sin(rad);
		line(mosaic, Point(center.x + x,center.y + y), Point(center.x - x,center.y - y), Scalar(255, 255, 255), 1);
	}
}

void MosaicData::DrawCrossHairsBlack(Mat &mosaic)
{
	int halfWidth = mosaic.cols / 2;
	int halfHeight = mosaic.rows / 2;
	double crossHairSize = _crossHairRadius * halfHeight;
	Point center(halfWidth, halfHeight);

	// Draw markers
	Scalar color = Scalar(200, 200, 200);
	int maxDim = max(mosaic.rows, mosaic.cols);
	int numMinorTicks = 5;
	double spacingMajorTick = 20.0;
	double spacingMinorTick = spacingMajorTick / numMinorTicks;
	double lengthMajorTick = 10.0;
	double lengthMinorTick = 5.0;
	
	for (double rad = PI / 4.0; rad < 2 * PI; rad += PI / 2.0)
	{
		double xStart = crossHairSize * cos(rad);
		double yStart = crossHairSize * sin(rad);
		double xEnd = maxDim * cos(rad);
		double yEnd = maxDim * sin(rad);
		line(mosaic, Point(center.x + xStart, center.y + yStart), Point(center.x + xEnd, center.y + yEnd), color, 2);
	
		double loc = crossHairSize;
		double xMajorTick = -lengthMajorTick*sin(rad);
		double yMajorTick = lengthMajorTick*cos(rad);
		double xMinorTick = -lengthMinorTick*sin(rad);
		double yMinorTick = lengthMinorTick*cos(rad);
		while (loc < maxDim)
		{
			// Draw major
			double xLoc = loc * cos(rad);
			double yLoc = loc * sin(rad);
			Point2d start = Point2d(center.x + xLoc + xMajorTick, center.y + yLoc + yMajorTick);
			Point2d end = Point2d(center.x + xLoc - xMajorTick, center.y + yLoc - yMajorTick);
			line(mosaic, start, end, color, 1);

			// Draw 4 minors
			for (int i = 1; i < numMinorTicks; i++)
			{
				double xLoc = (loc + (i*spacingMinorTick)) * cos(rad);
				double yLoc = (loc + (i*spacingMinorTick)) * sin(rad);
				Point2d start = Point2d(center.x + xLoc + xMinorTick, center.y + yLoc + yMinorTick);
				Point2d end = Point2d(center.x + xLoc - xMinorTick, center.y + yLoc - yMinorTick);

				line(mosaic, start, end, color, 1);
			}

			loc += spacingMajorTick;
		}
	}
}

void MosaicData::ApplyMask(Mat &mosaic)
{
	if (_mask.size() != mosaic.size())
	{
		_mask = Mat::ones(mosaic.size(), CV_8UC1);

		// Create the polygon
		int mosaicDim = min(mosaic.size().height, mosaic.size().width);
		double size = mosaicDim * 0.47;
		Point mosaicCenter = Point(mosaic.size().width / 2, mosaic.size().height / 2);
		Point corner_points[1][MASK_SIDES];
		for (int i = 0; i < MASK_SIDES; i++)
		{
			double angle = (i + 0.5) * 2 * PI / MASK_SIDES;
			double cosAngle = cos(angle);
			double sinAngle = sin(angle);
			corner_points[0][i] = mosaicCenter + Point(size * cosAngle, size * sinAngle);
		}
		const Point* ppt[1] = { corner_points[0] };
		int npt[] = { MASK_SIDES };

		fillPoly(_mask,
			ppt,
			npt,
			1,
			Scalar(0, 0, 0));
	}
	
	mosaic.setTo(Scalar(0, 0, 0), _mask);
}
