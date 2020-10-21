#include "stdafx.h"
#include "USBReader.h"
#include "CameraData.h"
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <log4cxx\logger.h>
#include <fstream>

log4cxx::LoggerPtr CUSBReader::logger = log4cxx::Logger::getLogger("USBReader");

const int CUSBReader::QueueSize = 8;
const int CUSBReader::MAX_QUEUE_SZ = 64;
const int CUSBReader::TimeOut = 500;
const int CUSBReader::BYTES_PER_PIXEL = 2;
int CUSBReader::PPX = 0;
const unsigned short FirstPixelValue = 0xAAA0;

CUSBReader::CUSBReader()
{
}


CUSBReader::~CUSBReader()
{
	StopCapture();

	LOG4CXX_INFO(logger, "Threads complete");
}

int CUSBReader::Initialize(std::vector<CameraData*> &cameras, std::string debugImageFolder, string simImageFolder)
{
	LOG4CXX_INFO(logger, "Initializing");
	
	m_sDebugImageFolder = debugImageFolder;
	m_sSimImageFolder = simImageFolder;

	m_nResetCount = 0;
	m_bInhibitRecovery = false;

	m_vCriticalSections.resize(cameras.size());

	for (int i = 0; i < cameras.size(); i++)
	{
		m_vCameras.push_back(cameras[i]);
		m_vSaveNextTransfer.push_back(false);
		m_vResetCameraMask.push_back(false);
		m_vCaptureThreads.push_back(NULL);
		m_vSimThreads.push_back(NULL);
		m_vSimTimers.push_back(NULL);
		m_vExitFlags.push_back(false);
		m_vSimImages.push_back(Mat());
		m_vStreamStatus.push_back(StreamStatus::Startup);
		m_vFWVersions.push_back("");
		m_vUSBDevices.push_back(NULL);

		InitializeCriticalSection(&m_vCriticalSections[i]);
	}

	return 0;
}

void CUSBReader::StartCapture()
{
	LOG4CXX_INFO(logger, "StartCapture()");

	OpenUSBDevices();

	for (int camera = 0; camera < m_vCameras.size(); camera++)
	{
		CaptureThreadParams *params = new CaptureThreadParams;
		params->pUSBReader = this;
		params->nCameraIndex = camera;

		if (!m_vCameras[camera]->IsCameraSimulated() && (m_vUSBDevices[camera] != NULL))
		{
			// Create a thread
			DWORD dwThreadID;
			m_vCaptureThreads[camera] = CreateThread(
				NULL,         // default security attributes
				0,            // default stack size
				(LPTHREAD_START_ROUTINE)&CUSBReader::CaptureThreadProc,
				(LPVOID)params,         // no thread function arguments
				0,            // default creation flags
				&dwThreadID); // receive thread identifier
		}
		else if (m_vCameras[camera]->IsCameraSimulated() && !m_sSimImageFolder.empty())
		{
			// Load the sim image
			int height = m_vCameras[camera]->GetSensorHeight();
			int width = m_vCameras[camera]->GetSensorWidth();
			Mat imgCapture = Mat::ones(height, width, CV_16U) * 4096;
			string fileName = m_sSimImageFolder + "\\" + m_vCameras[camera]->GetName() + ".raw";
			FILE *file = fopen(fileName.c_str(), "rb");
			if (file)
			{
				fread(imgCapture.data, 2, width * height, file);
				fclose(file);

				m_vSimImages[camera] = imgCapture;

				// Create the timer queue.
				m_vSimThreads[camera] = CreateTimerQueue();

				// Set a timer to call the timer routine for 30 FPS
				CreateTimerQueueTimer(&m_vSimTimers[camera], m_vSimThreads[camera],
					(WAITORTIMERCALLBACK)&CUSBReader::SimTimerProc, params, 33, 33, 0);
			}
		}	

		if (m_vCameras[camera]->IsCameraSimulated())
		{
			m_vFWVersions[camera] = "Sim";
		}
	}
}

void CUSBReader::StopCapture()
{
	LOG4CXX_INFO(logger, "StopCapture()");

	LOG4CXX_INFO(logger, "Setting exit flag for all threads");

	vector<HANDLE> captureThreads;
	for (int camera = 0; camera < m_vCameras.size(); camera++)
	{
		m_vExitFlags[camera] = true;
		if (m_vCaptureThreads[camera] != NULL)
		{
			captureThreads.push_back(m_vCaptureThreads[camera]);
		}
	}

	if (captureThreads.size() > 0)
	{
		LOG4CXX_INFO(logger, "Waiting for " << captureThreads.size() << " capture threads");

		WaitForMultipleObjects(captureThreads.size(), &captureThreads[0], true, INFINITE);
	}

	LOG4CXX_INFO(logger, "Waiting for " << captureThreads.size() << " capture threads complete");
	//LOG4CXX_INFO(logger, printf("Waiting for %d capture threads complete",captureThreads.size() ));  //javqui
	
	for (int camera = 0; camera < m_vCameras.size(); camera++)
	{
		if (m_vCaptureThreads[camera] != NULL)
		{
			m_vCaptureThreads[camera] = NULL;
			int height = m_vCameras[camera]->GetSensorHeight();
			int width = m_vCameras[camera]->GetSensorWidth();
			cv::Mat img = cv::Mat::zeros(height, width, CV_16U);
			m_vCameras[camera]->SetBayerImage(img, false, "");
		}
		else if (m_vSimThreads[camera])
		{
			DeleteTimerQueueTimer(m_vSimThreads[camera], m_vSimTimers[camera], INVALID_HANDLE_VALUE);
			DeleteTimerQueue(m_vSimThreads[camera]);
			m_vSimThreads[camera] = NULL;
		}

		m_vExitFlags[camera] = false;
	}

	CloseUSBDevices();
}

void CUSBReader::OpenUSBDevices()
{
	CloseUSBDevices();

	LOG4CXX_INFO(logger, "OpenUSBDevices()");

	CCyUSBDevice *USBDevice = new CCyUSBDevice();
	if (USBDevice == NULL)
	{
		LOG4CXX_ERROR(logger, "OpenUSBDevices unable to open USB device");
		return;
	}

	int n = USBDevice->DeviceCount();
	LOG4CXX_INFO(logger, "OpenUSBDevices device count = " << n);
	delete USBDevice;

	vector<CCyUSBDevice *> devices;
	for (int i = 0; i < n; i++)
	{
		LOG4CXX_INFO(logger, "OpenUSBDevices opening device #" << i);
		CCyUSBDevice *USBDevice = new CCyUSBDevice();
		USBDevice->Open(i);

		bool bFound = false;
		char desc[256];
		sprintf(desc, "(0x%x - 0x%x) %s", USBDevice->VendorID, USBDevice->ProductID, USBDevice->FriendlyName);
		string strDeviceData = desc;
		string strDeviceName = USBDevice->DeviceName;
		std::wstring ws(USBDevice->Manufacturer);
		string strVersion(ws.begin(), ws.end());

		if (strDeviceData.find("Streamer") != string::npos)
		{
			LOG4CXX_INFO(logger, "OpenUSBDevices found: " << strDeviceName << " " << strDeviceData << " VERS: " << strVersion);

			for (int camera = 0; camera < m_vCameras.size(); camera++)
			{
				if (strDeviceName.find(m_vCameras[camera]->GetName()) != string::npos)
				{
					if (m_vUSBDevices[camera] == NULL)
					{
						LOG4CXX_INFO(logger, "OpenUSBDevices " << strDeviceName << " matches camera " << CameraNames[camera]);

						bFound = true;
						m_vUSBDevices[camera] = USBDevice;
						m_vStreamStatus[camera] = StreamStatus::Opened;
						if (strVersion.find("Vers") == 0)
						{
							m_vFWVersions[camera] = "V" + strVersion.substr(4, 3);
						}
						else
						{
							m_vFWVersions[camera] = "V0.0";
						}
					}
					else
					{
						LOG4CXX_ERROR(logger, "OpenUSBDevices unable to to assign " << strDeviceName << " to already assigned camera " << CameraNames[camera]);
					}
				}
			}
			if (!bFound)
			{
				LOG4CXX_ERROR(logger, "OpenUSBDevices could not find a matching camera for " << strDeviceName);
			}
		}
	}

	// Mark any USB that couldn't open as 
	for (int i = 0; i < 5; i++)
	{
		if (m_vStreamStatus[i] != StreamStatus::Opened)
		{
			LOG4CXX_ERROR(logger, "Could not open USB device " << i << " for camera " << m_vCameras[i]->GetName() << " " << CameraNames[i]);
			m_vStreamStatus[i] = StreamStatus::Closed;
		}
	}
}

void CUSBReader::CloseUSBDevices()
{
	LOG4CXX_INFO(logger, "CloseUSBDevices()");

	for (int i = 0; i < m_vUSBDevices.size(); i++)
	{
		if (m_vUSBDevices[i] != NULL)
		{
			LOG4CXX_INFO(logger, "Closing device " << i << " for camera " << CameraNames[i]);

			m_vUSBDevices[i]->Reset();
			m_vUSBDevices[i]->Close();
			delete m_vUSBDevices[i];
			m_vUSBDevices[i] = NULL;
		}

		m_vStreamStatus[i] = StreamStatus::Startup;
	}
}



void CUSBReader::ResetAllUSBChips()
{
	LOG4CXX_INFO(logger, "ResetAllUSBChips sending reset to " << m_vUSBDevices.size() << " devices");  // javqui : check why if found more USB device on this version
	for (int i = 0; i < m_vUSBDevices.size(); i++)
	{
		if (m_vUSBDevices[i] != NULL)
		{
			LOG4CXX_INFO(logger, "ResetAllUSBChips resetting device " << i);

			//devices[i]->Reset();
			if (m_vUSBDevices[i]->ControlEndPt)
			{
				SendVendorCommand(m_vUSBDevices[i]->ControlEndPt, 0x90);
			}
			else
			{
				LOG4CXX_INFO(logger, "ResetAllUSBChips unable to reset device " << i << " ControlEndPt is NULL");
			}
		}
		LOG4CXX_INFO(logger, "ResetAllUSBChips ignoring disconnected device " << i);
	}
	
	LOG4CXX_INFO(logger, "ResetAllUSBChips complete " <<  m_vUSBDevices.size() << " devices");
	//LOG4CXX_INFO(logger, printf("ResetAllUSBChips complete %d devices", m_vUSBDevices.size()) );  // javqui fast optioin
	//LOG4CXX_INFO(logger, "ResetAllUSBChips complete 6 devices"); // javqui test
}

void CUSBReader::SaveNextTransfer(string folder)
{
	m_sDebugImageFolder = folder;
	for (int i = 0; i < m_vSaveNextTransfer.size(); i++)
		m_vSaveNextTransfer[i] = true;

}

void CUSBReader::InhibitRecovery(bool inhibit)
{
	if (m_bInhibitRecovery != inhibit)
	{
		LOG4CXX_INFO(logger, "InhibitRecovery(" << (inhibit ? "true" : "false") << ")");
		m_bInhibitRecovery = inhibit;
	}
}

CompStatus CUSBReader::GetStatus() 
{ 
	bool bAllStreaming = true;
	bool bSomeClosed = false;

	for (int i = 0; i < m_vStreamStatus.size(); i++)
	{
		if (m_vCameras[i]->IsCameraSimulated())
		{
			continue;
		}

		if (m_vStreamStatus[i] != StreamStatus::Streaming)
		{
			bAllStreaming = false;
		}
		if (m_vStreamStatus[i] == StreamStatus::Closed)
		{
			bSomeClosed = true;
		}
	}

	CompStatus status = CompStatus::eError;
	if (bSomeClosed)
	{
		status = CompStatus::eFatal;
	}
	else if (bAllStreaming)
	{
		status = CompStatus::eOK;
	}

	if (status == CompStatus::eOK)
	{
		m_nResetCount = 0;
	}
	
	return status;
}

std::string CUSBReader::GetFWVersion(int camera)
{
	string version = "NONE";
	if ((camera >= 0) && (camera < m_vFWVersions.size()))
	{
		version = m_vFWVersions[camera];
	}
	return version;
}

float CUSBReader::GetFWVersionNum(int camera)
{
	float ver = 0.0;

	string version = GetFWVersion(camera);
	if (version[0] == 'V')
	{
		string sVer = version.substr(1, 3);
		ver = atof(sVer.c_str());
	}

	return ver;
}


void CUSBReader::ResetCameras(unsigned int mask)
{
	for (int i = 0; i < m_vResetCameraMask.size(); i++)
	{
		if (mask & (1 << i))
		{
			m_vResetCameraMask[i] = true;
		}
	}
}

UINT CUSBReader::CaptureThread(int index)
{
	LOG4CXX_INFO(logger, "Capture thread " << index << " is starting");

	CameraData *pCamera = m_vCameras[index];
	CCyUSBDevice *USBDevice = m_vUSBDevices[index];

	CCyUSBEndPoint* EndPt = NULL;
	CCyControlEndPoint   *CtrlEndPt = NULL;

	// Allocate the arrays needed for queueing
	PUCHAR			*buffers = new PUCHAR[QueueSize];
	PUCHAR			*contexts = new PUCHAR[QueueSize];
	OVERLAPPED		inOvLap[MAX_QUEUE_SZ];

	int height = pCamera->GetSensorHeight();
	int width = pCamera->GetSensorWidth();
	int countEmptyXfers = 0;
	int countSplitFrame = 0;
	int countCameraRestarts = 0;
	bool bRestartCamera = false;
	const int LIMIT_CAMERA_RESTARTS = 5;
	const int LIMIT_EMPTY_XFERS = 5;
	const int NUM_FRAMES_TO_IGNORE_AFTER_START = 10;
	int i = 0;
	int countIgnoreXfersAfterStart = NUM_FRAMES_TO_IGNORE_AFTER_START;
	cv::Mat imgCapture = cv::Mat::zeros(height, width, CV_16U);

	long len = BYTES_PER_PIXEL * height * width; // Each xfer request will get PPX isoc packets
	int bytesToCopy = len;

	// Allocate all the buffers for the queues
	for (int i = 0; i < QueueSize; i++)
	{
		buffers[i] = new UCHAR[len];
		inOvLap[i].hEvent = CreateEvent(NULL, false, false, NULL);

		memset(buffers[i], 0xEF, len);
	}

	// Determine if camera should reboot for undersized packet
	BYTE *pCaptureData = imgCapture.data;
	bool bRestartOnWrongSize = true;
	float version = GetFWVersionNum(index);
	if (version > 1.0)
	{
		bRestartOnWrongSize = false;

		// Handle side camera sample shift
		if (pCamera->GetCameraType() != eFront)
		{
			pCaptureData += 2;
			bytesToCopy -= 2;
		}
	}
	LOG4CXX_INFO(logger, "Capture thread " << index << " camera version is " << version	<< ". RestartOnWrongSize is " << (bRestartOnWrongSize ? "true" : "false"));
	//LOG4CXX_INFO(logger, printf("Capture thread %d camera version is %d. RestartOnWrongSize is %s",index,version, (bRestartOnWrongSize ? "true" : "false"))); // javqui
	
	int eptCnt = USBDevice->EndPointCount();

	// Fill the EndPointsBox
	for (int e = 1; e < eptCnt; e++)
	{
		EndPt = USBDevice->EndPoints[e];
		// INTR, BULK and ISO endpoints are supported.
		if ((EndPt->Attributes >= 1) && (EndPt->Attributes <= 3))
		{
			CtrlEndPt = USBDevice->ControlEndPt;
			break;
		}
	}

	PPX = ceil(BYTES_PER_PIXEL * height * width / EndPt->MaxPktSize);

	if (!SetupXferLoop(USBDevice, EndPt, CtrlEndPt, buffers, contexts, inOvLap, len))
	{
		PCHAR			errorStr = new CHAR[128];
		USBDevice->UsbdStatusString(EndPt->UsbdStatus, errorStr);
		LOG4CXX_ERROR(logger, "Capture thread " << index << " unable to setup " << pCamera->GetName() << " NTSTATUS = " << std::hex << EndPt->NtStatus << ", UsbdStatus = " << errorStr);

		m_vStreamStatus[index] = StreamStatus::Error;
		m_vExitFlags[index] = true;
	}

	while (!m_vExitFlags[index])
	{
		try
		{
			bool success = true;

			// Reset this each time through because
			// FinishDataXfer may modify it
			long rLen = len;

			if (!EndPt->WaitForXfer(&inOvLap[i], TimeOut))
			{
				PCHAR			errorStr = new CHAR[128];
				USBDevice->UsbdStatusString(EndPt->UsbdStatus, errorStr);
				LOG4CXX_INFO(logger, "Capture thread " << index << " for " << pCamera->GetName() << " timeout NTSTATUS = " << std::hex << EndPt->NtStatus << ", UsbdStatus = " << errorStr);

				if (bRestartOnWrongSize)
				{
					success = false;
				}
				else
				{
					m_vResetCameraMask[index] = true;

					// Resetting on timeout seems to cause issue with driver disappearing, so just exit instead so that reset line will be asserted for all devices
					// Nevermind...causes constant resets with ESD hits
					//m_vExitFlags[index] = true;
					//m_vStreamStatus[index] = StreamStatus::Error;
				}
			}
			else if (EndPt->FinishDataXfer(buffers[i], rLen, &inOvLap[i], contexts[i]))
			{
				unsigned short value = ((unsigned short *)buffers[i])[0];
				bool bFirstPixelOK = ((value == FirstPixelValue) || (pCamera->GetCameraType() != eFront)) || (!m_bFrontSplitImageDetectionEnabled);

				if (bFirstPixelOK && (rLen == BYTES_PER_PIXEL*height*width))
				{
					if (m_vStreamStatus[index] != StreamStatus::Streaming)
					{
						LOG4CXX_INFO(logger, "Capture thread " << index << " for " << pCamera->GetName() << " changing state to Streaming");
						countCameraRestarts = 0;
						m_vStreamStatus[index] = StreamStatus::Streaming;
					}

					if (pCamera->IsImageOutOfRange() && (pCamera->GetCameraState() == CameraState::eRunning))
					{
						LOG4CXX_ERROR(logger, "Capture thread " << index << " for " << pCamera->GetName() << " detected image out of range. Marking camera for restart");
						pCamera->ResetImageOutOfRange();
						bRestartCamera = true;
					}
					else
					{
						countIgnoreXfersAfterStart = NUM_FRAMES_TO_IGNORE_AFTER_START;
						countEmptyXfers = 0;
						memcpy(pCaptureData, buffers[i], bytesToCopy);

						pCamera->SetBayerImage(imgCapture, m_vSaveNextTransfer[index],
							m_sDebugImageFolder);
					}

					if (m_vSaveNextTransfer[index])
					{
						string fileName = m_sDebugImageFolder + "\\" + pCamera->GetName() + ".raw";
						FILE *file = fopen(fileName.c_str(), "wb");
						if (file)
						{
							fwrite(imgCapture.data, 2, height*width, file);
							fflush(file);
							fclose(file);
						}
						m_vSaveNextTransfer[index] = false;
					}
				}
				else
				{
					if (countIgnoreXfersAfterStart <  NUM_FRAMES_TO_IGNORE_AFTER_START)
					{
						LOG4CXX_INFO(logger, "Capture thread " << index << " for " << pCamera->GetName() << " ignoring after restart bytes = " << rLen << " # " << countIgnoreXfersAfterStart);
						countIgnoreXfersAfterStart++;
					}
					else if (rLen == 16)
					{
						countEmptyXfers++;
						LOG4CXX_INFO(logger, "Capture thread " << index << " for " << pCamera->GetName() << " received bytes = " << rLen << " # " << countEmptyXfers);

						if (countEmptyXfers == LIMIT_EMPTY_XFERS)
						{
							LOG4CXX_INFO(logger, "Capture thread " << index << " setting camera restart flag");
							bRestartCamera = true;
						}
					}
					else if (!bFirstPixelOK)
					{
						countSplitFrame++;
						LOG4CXX_ERROR(logger, "Capture thread " << index << " first pixel is  " << value << " instead of " << FirstPixelValue << " # " << countSplitFrame);

						if (countSplitFrame == 3)
						{
							LOG4CXX_INFO(logger, "Capture thread " << index << " Resetting stream due to split frames detected");
							countSplitFrame = 0;
							m_vResetCameraMask[index] = true;
						}
					}
					else
					{
						LOG4CXX_INFO(logger, "Capture thread " << index << " for " << pCamera->GetName() << " ignoring frame with byte = " << rLen);

						countEmptyXfers = 0;
						if (bRestartOnWrongSize)
						{
							LOG4CXX_INFO(logger, "Capture thread " << index << " Resetting camera due to wrong size frame");
							m_vResetCameraMask[index] = true;
						}
					}
				}

				// Re-submit this queue element to keep the queue full
				contexts[i] = EndPt->BeginDataXfer(buffers[i], len, &inOvLap[i]);
				if ((EndPt->NtStatus == 0) && (EndPt->UsbdStatus == 0))
				{

				}
				else if (bRestartOnWrongSize)
				{
					success = false;
				}
			}
			else
			{
				if (bRestartOnWrongSize)
				{
					success = false;
				}
			}

			i++;
			if (i == QueueSize)
			{
				i = 0;
			}

			if (!success && !m_vResetCameraMask[index])
			{
				LOG4CXX_INFO(logger, "Capture thread " << index << " frame error recovery for " << pCamera->GetName());
				AbortXferLoop(EndPt, CtrlEndPt, QueueSize, len, buffers, contexts, inOvLap);
				if (!SetupXferLoop(USBDevice, EndPt, CtrlEndPt, buffers, contexts, inOvLap, len))
				{
					PCHAR			errorStr = new CHAR[128];
					USBDevice->UsbdStatusString(EndPt->UsbdStatus, errorStr);
					LOG4CXX_INFO(logger, "Capture thread " << index << " unable to recover " << pCamera->GetName() << " NTSTATUS = " << std::hex << EndPt->NtStatus << ", UsbdStatus = " << errorStr);
					m_vExitFlags[index] = true;
				}
				i = 0;
			}

			if (m_vResetCameraMask[index])
			{
				LOG4CXX_INFO(logger, "Capture thread " << index << " resetting USB device for camera " << pCamera->GetName());
				AbortXferLoop(EndPt, CtrlEndPt, QueueSize, len, buffers, contexts, inOvLap);
				//SendVendorCommand(CtrlEndPt, 0x90);
				m_vUSBDevices[index]->Reset();
				if (!SetupXferLoop(USBDevice, EndPt, CtrlEndPt, buffers, contexts, inOvLap, len))
				{
					PCHAR			errorStr = new CHAR[128];
					USBDevice->UsbdStatusString(EndPt->UsbdStatus, errorStr);
					LOG4CXX_ERROR(logger, "Capture thread " << index << " unable to start " << pCamera->GetName() << " NTSTATUS = " << std::hex << EndPt->NtStatus << ", UsbdStatus = " << errorStr);

					m_vStreamStatus[index] = StreamStatus::Error;
					m_vExitFlags[index] = true;
				}
				else
				{
					i = 0;
					m_vResetCameraMask[index] = false;

					LOG4CXX_INFO(logger, "Capture thread " << index << " setting camera restart flag");
					bRestartCamera = true;
				}
			}

			if (bRestartCamera && !m_bInhibitRecovery)
			{
				if (countCameraRestarts < LIMIT_CAMERA_RESTARTS)
				{
					LOG4CXX_INFO(logger, "Capture thread " << index << " restarting camera " << pCamera->GetName() << " Count=" << countCameraRestarts);

					countCameraRestarts++;
					countEmptyXfers = 0;
					countIgnoreXfersAfterStart = 0;

					// Send the camera the start command
					pCamera->StartCamera(false);
					m_vStreamStatus[index] = StreamStatus::Recovery;
				}
				else
				{
					LOG4CXX_ERROR(logger, "Capture thread exiting due to excessive camera restarts");
					m_vExitFlags[index] = true;
				}
			}
			bRestartCamera = false;
		}
		catch (...)
		{
			LOG4CXX_INFO(logger, "Capture thread " << index << " exception for camera " << pCamera->GetName());
		}
	}

	if (m_vStreamStatus[index] != StreamStatus::Error)
	{
		AbortXferLoop(EndPt, CtrlEndPt, QueueSize, len, buffers, contexts, inOvLap);
	}

	m_vStreamStatus[index] = StreamStatus::Closed;

	for (int j = 0; j < QueueSize; j++)
	{
		CloseHandle(inOvLap[j].hEvent);

		delete[] buffers[j];
	}

	delete[] buffers;
	delete[] contexts;

	LOG4CXX_INFO(logger, "Capture thread " << index << " exiting for camera " << pCamera->GetName());


	return 0;
}

void CUSBReader::SimTimer(int camera)
{
	int index = camera;
	CameraData *pCamera = m_vCameras[camera];
	
	if (!m_vExitFlags[index])
	{
		if (TryEnterCriticalSection(&m_vCriticalSections[index]))
		{
			try
			{
				pCamera->SetBayerImage(m_vSimImages[index].clone(), m_vSaveNextTransfer[index],
					m_sDebugImageFolder);
				m_vSaveNextTransfer[index] = false;
			}
			catch (...)
			{
				LOG4CXX_ERROR(logger, "Sim proc " << index << " exception for camera " << pCamera->GetName());
			}

			LeaveCriticalSection(&m_vCriticalSections[index]);
		}
	}
}

void CUSBReader::SendVendorCommand(CCyControlEndPoint   *CtrlEndPt, UCHAR command)
{
	LOG4CXX_INFO(logger, "SendVendorCommand " << std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)command);

	CtrlEndPt->Target = TGT_DEVICE;
	CtrlEndPt->ReqType = REQ_VENDOR;
	CtrlEndPt->Direction = DIR_TO_DEVICE;
	CtrlEndPt->ReqCode = command; // Get Descriptor Standard Request
	CtrlEndPt->Value = 0; // Configuration Descriptor
	CtrlEndPt->Index = 1;

	LONG len = 0;
	UCHAR *buf = new UCHAR[1];
	CtrlEndPt->XferData(buf, len);
}

bool CUSBReader::SetupXferLoop(CCyUSBDevice *USBDevice,
	CCyUSBEndPoint *EndPt, CCyControlEndPoint *CtrlEndPt,
	PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED inOvLap[], int len)
{
	m_CriticalSection.Lock();

	bool retVal = true;

	EndPt->SetXferSize(len);

	// Queue-up the first batch of transfer requests
	for (int i = 0; i < QueueSize; i++)
	{
		contexts[i] = EndPt->BeginDataXfer(buffers[i], len, &inOvLap[i]);

		if (EndPt->NtStatus || EndPt->UsbdStatus) // BeginDataXfer failed
		{
			retVal = false;
			break;
		}
	}

	// Start the camera grabbing if no error
	if (retVal)
	{
		SendVendorCommand(CtrlEndPt, 0x99);
	}

	m_CriticalSection.Unlock();

	return retVal;
}

void CUSBReader::AbortXferLoop(CCyUSBEndPoint *EndPt, CCyControlEndPoint   *CtrlEndPt,
	int pending, long length, PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED inOvLap[])
{
	m_CriticalSection.Lock();

	LOG4CXX_INFO(logger, "AbortXferLoop");

	SendVendorCommand(CtrlEndPt, 0x88);

	LOG4CXX_INFO(logger, "AbortXferLoop finished sending 0x88");

	EndPt->Abort();

	for (int j = 0; j < QueueSize; j++)
	{
		if (j < pending)
		{
			try
			{
				LOG4CXX_INFO(logger, "AbortXferLoop WaitForXfer " << j);

				if (EndPt->WaitForXfer(&inOvLap[j], 0))
				{
					LOG4CXX_INFO(logger, "AbortXferLoop FinishDataXfer " << j);

					long len = length;
					EndPt->FinishDataXfer(buffers[j], len, &inOvLap[j], contexts[j]);

					LOG4CXX_INFO(logger, "AbortXferLoop FinishDataXfer " << j << " complete with length " << len);
				}
			}
			catch (...)
			{

			}
		}
	}

	EndPt->Reset();

	LOG4CXX_INFO(logger, "AbortXferLoop complete");

	m_CriticalSection.Unlock();
}