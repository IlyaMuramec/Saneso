#include "stdafx.h"
#include "ImageBuilder.h"
#include "CameraData.h"

log4cxx::LoggerPtr CImageBuilder::logger = log4cxx::Logger::getLogger("ImageBuilder");

using namespace cv;

#define FRAME_SAMPLED 1
#define NUM_VSYNC_CONSEQ 2
#define NUM_LOW_HSYNC_PIXELS 93
#define NUM_PIXELS_PER_ROW 734

CImageBuilder::CImageBuilder(CameraData *camera, std::string debugImageFolder)
{
	m_sDebugImageFolder = debugImageFolder;
	m_nCurrentFrame = 0;
	m_bSaveNextTransfer = false;
	m_pCamera = camera;

	m_nSensorHeight = camera->GetSensorHeight();
	m_nSensorWidth = camera->GetSensorWidth();
	
	m_BayerImage = Mat::zeros(m_nSensorHeight, m_nSensorWidth, CV_8U);
	
	Reset();

	m_FrameState = eUnknown;
}

CImageBuilder::~CImageBuilder()
{
}

void CImageBuilder::Reset()
{
	m_nRowIndex = 0;
	m_nColIndex = 0;
	m_nHSyncCount = 0;
	m_nVSyncCount = 0;
	m_bFrameValid = false;
	m_bLineValid = false;
	m_pPxlRow = m_BayerImage.ptr(0);
}

void CImageBuilder::AddPixel(unsigned short pixelVal, bool vsync, bool hsync, bool filterFrames)
{
#if 1
	m_nSampleCount++;

	switch (m_FrameState)
	{
	case eUnknown:
		if (!vsync && !hsync)
		{
			m_FrameState = eOutOfFrame;
		}
		break;

	case eOutOfFrame:
		if (vsync)
		{
			m_nVSyncCount++;
		}
		else
		{
			m_nVSyncCount = 0;
		}

		if (m_nVSyncCount >= NUM_VSYNC_CONSEQ)
		{
			m_nVSyncCount = 0;
			m_nCurrentFrame++;
			if ((m_nCurrentFrame % FRAME_SAMPLED) == 0)
			{
				m_FrameState = eOutOfLine;
				m_nSampleCount = (NUM_VSYNC_CONSEQ - NUM_LOW_HSYNC_PIXELS - 1);
			}
			else
			{
				m_FrameState = eInSkipFrame;
			}
		}
		break;

	case eInSkipFrame:
		if (!vsync)
		{
			Reset();
			m_FrameState = eOutOfFrame;
		}
		break;

	case eOutOfLine:
		if (!vsync)
		{
			SendImage();
			m_FrameState = eOutOfFrame;
		}
		else if (hsync)
		{
			m_FrameState = eInLine;

			// Check for error sample
			if (filterFrames && ((m_nSampleCount % NUM_PIXELS_PER_ROW) != 0))
			{
				Reset();
				m_FrameState = eUnknown;
			}
		}
		
		break;
	case eInLine:
		if (hsync)
		{
			// For some reason there are 641 horizontal pixels input
			if (m_nColIndex < m_nSensorWidth)
			{
				ushort pixVal16 = (pixelVal & 0xfff);
				byte pixlVal = (pixVal16 >> 4);
				m_pPxlRow[m_nColIndex] = pixlVal;
			}

			m_nColIndex++;
		}
		else
		{
			m_nColIndex = 0;
			m_nRowIndex++;
			if (m_nRowIndex < m_nSensorHeight)
			{
				m_pPxlRow = m_BayerImage.ptr(m_nRowIndex);
			}
			m_FrameState = eOutOfLine;
		}
		if (m_nRowIndex == m_nSensorHeight)
		{
			SendImage();
			m_FrameState = eUnknown;  // Need to wait for next out of frame
			return;
		}

		break;
	}
#else
	if (vsync && (m_nVSyncCount > 10))
		m_bFrameValid = true;

	if (m_bFrameValid)
	{
		if (hsync && (m_nHSyncCount > 10))
		{
			m_bLineValid = true;
			m_nHSyncCount = 0;
			m_pPxlRow = m_BayerImage.ptr(m_nRowIndex);
		}

		if (m_bLineValid)
		{
			ushort pixVal16 = (pixelVal & 0xfff);
			byte pixlVal = (pixVal16 >> 4);

			// For some reason there are 641 horizontal pixels input
			m_pPxlRow[m_nColIndex] = pixlVal;

			m_nColIndex++;
			if (m_nColIndex == m_pCamera->GetSensorWidth())
			{
				m_bLineValid = false;
				m_nHSyncCount = 0;
				m_nColIndex = 0;
				m_nRowIndex++;
				if (m_nRowIndex == m_pCamera->GetSensorHeight())
				{
					m_bFrameValid = false;
					m_nHSyncCount = 0;
					m_nRowIndex = 0;
				}
			}
		}

		if (!m_bFrameValid)
		{
			m_pCamera->SetBayerImage(m_BayerImage, m_bSaveNextTransfer);
			if (m_bSaveNextTransfer)
			{
				cv::Mat testImage;;
				cv::cvtColor(m_BayerImage, testImage, CV_BayerGR2BGR);
				char fname[128];
				sprintf_s(fname, "RawImage_%s.bmp", m_pCamera->GetName().c_str());
				cv::imwrite(fname, m_BayerImage);
				sprintf_s(fname, "TestImage_%s.bmp", m_pCamera->GetName().c_str());
				cv::imwrite(fname, testImage);
				m_bSaveNextTransfer = false;
			}
			Reset();
			return;
		}
	}

	if (!vsync)
		m_nVSyncCount++;
	else
		m_nVSyncCount = 0;

	if (!hsync)
		m_nHSyncCount++;
	else
		m_nHSyncCount = 0;
#endif

}

void CImageBuilder::SendImage()
{
	m_pCamera->SetBayerImage(m_BayerImage, m_bSaveNextTransfer, "C:\\NGE\\DebugImages");
	if (m_bSaveNextTransfer)
	{
		cv::Mat testImage;;
		cv::cvtColor(m_BayerImage, testImage, CV_BayerGR2BGR);
		char fname[128];
		sprintf_s(fname, "%s\\RawImage_%s_%02d.bmp", m_sDebugImageFolder.c_str(), m_pCamera->GetName().c_str(), m_nCurrentFrame);
		cv::imwrite(fname, m_BayerImage);
		sprintf_s(fname, "%s\\TestImage_%s_%02d.bmp", m_sDebugImageFolder.c_str(), m_pCamera->GetName().c_str(), m_nCurrentFrame);
		cv::imwrite(fname, testImage);
		m_bSaveNextTransfer = false;
	}
	Reset(); 
}


