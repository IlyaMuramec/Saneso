#pragma once

#define BOOL int // Prevent from being changed by Pentek

#include "ptk_osdep.h"      /* This should be included first */                          

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <vector>
#include <queue>
#include <log4cxx\logger.h>
#include "afxmt.h"

#include "ptk716x.h"
#include "716x.h"         /* 716x-family general header */
#include "716xcmdline.h"  /* For command line argument processing */
#include "716xview.h"     /* For signal Analyzer */
#include "ImageBuilder.h"
#include "readerwriterqueue.h"


/* MODULE_RESOURCES - module resources structure, values set by
* deviceFindAndOpen(), contains module ID, BAR addresses,
* etc., as follows:
*     moduleId          - actual module ID
*     slot              - module slot
*     hDev              - device handle
*     BAR0Base          - BAR 0 base address
*     BAR2Base          - BAR 2 base address
*     BAR4Base          - BAR 4 base address
*     p716xRegs         - pointer to the module address table
*     p716xBrdResrc     - pointer to the board resource table
*     p716xGlobalParams - global parameter table pointer
*     p716xDiginParams  - DIGIN channel parameter table pointer
*                         (one for each channel)
*     nextDev           - pointer to next module resource table
*                         (linked list)
*/
typedef struct MODULE_RESRC
{
	DWORD                 moduleId;
	DWORD                 slot;
	PVOID                 hDev;
	unsigned int          dataDir;
	BAR_ADDR              BAR0Base;
	BAR_ADDR              BAR2Base;
	BAR_ADDR              BAR4Base;
	P716x_REG_ADDR        p716xRegs;
	P716x_BOARD_RESOURCE  p716xBrdResrc;
	P716x_GLOBAL_PARAMS   p716xGlobalParams;
	P716x_DIGIO_RX_CHAN_PARAMS p716xDigioRxParams[P716x_MAX_DIGIO_RX_CHANS];
	void                 *nextDev;
} MODULE_RESRC;


/* EXIT_HANDLE_RESRC - exit handler resource table, contains information
* needed by the exit handler to clean up before exit.
*     exitCode     - exit code
*     hDevPtr      - device handle pointer
*     modResrcBase - module resource table base address
*     libStatPtr   - library status pointer
*     intrStatPtr  - interrupt status pointer
*     dmaHandlePtr - DMA handle pointer
*     dmaDataBuf   - DMA buffer pointer
*     p716xRegs    - register address table pointer
*     channelPtr   - channel number pointer
*     ifcArgs      - OS-dependent resource structure pointer
*/
typedef struct EXIT_HANDLE_RESRC
{
	int                  exitCode;
	PVOID               *hDevPtr;
	MODULE_RESRC        *modResrc;
	DWORD               *libStatPtr;
	DWORD               *intrStatPtr;
	PTK716X_DMA_HANDLE **dmaHandlePtr;
	PTK716X_DMA_BUFFER  *dmaDataBuf[4];
	IFC_ARGS            *ifcArgs;

} EXIT_HANDLE_RESRC;


void trigIntHandler(PVOID               hDev,
	LONG                intSource,
	PVOID               pData,
	PTK716X_INT_RESULT *pIntResult);
int  PTKHLL_DeviceInit(MODULE_RESRC *moduleResrc);
void PTKHLL_ViewerSetControlWord(DWORD               moduleId,
	P716x_VIEW_CONTROL *viewCtrlWord,
	P716x_CMDLINE_ARGS *progParams);
void PTKHLL_SetProgramOptions(P716x_CMDLINE_ARGS   *argParams,
	P716x_BOARD_RESOURCE *brdResrc);
int  exitHandler(EXIT_HANDLE_RESRC *ehResrc);

void displayMetaData(P716x_DIGIO_RX_META_DATA_PARAMS *metaData,
	FILE                      *outPtr);

class CameraData;
class CPentekReader
{
public:
	CPentekReader();
	~CPentekReader();

	int Initialize(std::vector<CameraData*> &cameras, std::string debugImageFolder);
	void Close();
	void LoadSimulationData();

	static std::string GetErrorMessage(int index);
	CameraData *GetCamera(int index)	{ return m_vCameras[index]; }


	IFC_ARGS *GetIFCArgs() { return &(ifcCurrent); }
	EXIT_HANDLE_RESRC *GetExitHandleResrc() { return &(exitHdlResrc); }
	P716x_DIGIO_RX_META_DATA *GetMetaData(int index)	{ return metaData[index]; }
	PTK716X_DMA_BUFFER *GetRxDataBuffer(int index)		{ return &(rxDataBuf[index]); }
	MODULE_RESRC *GetModuleResrc()						{ return moduleResrc; }
	P716x_CMDLINE_ARGS *GetProgParams()					{ return &progParams; }
	bool IsExiting()									{ return m_bExiting; }
	bool IsSimulationEnabled()							{ return m_bSimulateFrames; }

	bool IsFilterFramesEnabled()					{ return m_bFilterFrames; }
	void EnableFilterFrames(bool enable)			{ m_bFilterFrames = enable; }

	int GetCameraCapture()								{ return m_nCameraCapture; }
	void SetCameraCapture(int camera)					{ m_nCameraCapture = camera; }

	std::string GetVersion()							{ return m_sVersion; }

	void SaveNextTransfer();
	bool IsSaveNextTransferOn();
	int WriteBufToFile(DWORD *buf,
							DWORD  size,
							char  *progId);

	void ProcessAllCameraBuffer(unsigned long *recvImage, unsigned int bufferLoc);

	void ResetAllImages();

	DWORD *GetFreeBuffer();
	DWORD *GetFullBuffer();
	void AddFreeBuffer(DWORD *ptr);
	void AddFullBuffer(DWORD *ptr);
	void SaveFullBuffers();

	static log4cxx::LoggerPtr logger;

private:
	static UINT CaptureThread(LPVOID param);
	static UINT ProcessingThread(LPVOID param);

	int exitHandler(EXIT_HANDLE_RESRC *ehResrc);
	
	MODULE_RESRC          *moduleResrc;
	P716x_CMDLINE_ARGS     progParams;
	EXIT_HANDLE_RESRC      exitHdlResrc;
	IFC_ARGS               ifcCurrent;
	DWORD                  libStat;
	DWORD                  intrStat;
	DWORD                  bufStat;
	PTK716X_DMA_HANDLE    *dmaHandle;
	PTK716X_DMA_BUFFER     rxDataBuf[2];
	PTK716X_DMA_BUFFER     metaDataBuf[2];
	P716x_DIGIO_RX_META_DATA        *metaData[2];
	std::vector<CameraData*> m_vCameras;
	std::vector<CImageBuilder*> m_vImageBuilders;

	CCriticalSection	m_FreeCriticalSection;
	CCriticalSection	m_FullCriticalSection;
	std::queue<DWORD*> m_vFreeBuffers;
	std::queue<DWORD*> m_vFullBuffers;
	moodycamel::ReaderWriterQueue<DWORD*> m_FreeBufferQueue;
	moodycamel::ReaderWriterQueue<DWORD*> m_FullBufferQueue;
	bool					m_bExiting;
	HANDLE					m_hCaptureThread;
	HANDLE					m_hProcessingThread;
	bool					m_bFilterFrames;
	bool					m_bSimulateFrames;
	int						m_nCameraCapture;
	std::string				m_sVersion;
	std::string				m_sDebugImageFolder;
	bool					m_bSaveNextTransfer;
};

