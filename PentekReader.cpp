#include "stdafx.h"
#include "PentekReader.h"
#include "CameraData.h"
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <log4cxx\logger.h>
#include <fstream>


/* Program control defines - the following defines are provided to alter
* program control for debug or evaluation purposes.
*
* PROG_DEBUG - set to 1 to save the data in the buffer to a file
* Default = 0.
*
* XFER_DEBUG - set to 1 to pause after each transfer.
* Default = 0.
*
* REG_DUMP - set to 1 to dump the SFPDP registers and related RX and TX
* channel registers to file sfpdpregs.txt.
* Default = 0.
*/
#define PROG_DEBUG   0
#define XFER_DEBUG   1	
#define REG_DUMP     1
#define SPEED_TEST	 1

log4cxx::LoggerPtr CPentekReader::logger = log4cxx::Logger::getLogger("PentekReader");

/* PROGRAM_ID - this character string is used for output file names and
* output messages.
*/
char *PROGRAM_ID = "digio_rx";


/* LOOP_COUNT - this constant specifies the number of loops that this
* program will loop before the program completes and exits.
*/
const DWORD LOOP_COUNT = 1000;

const int NUM_PROC_BUFFERS = 20;

/* DATA_PACK_MODE - selects the data mode of data for the channel.
* Select one of the following:
*    P716x_DIGIN_DATA_CTRL_PACK_MODE_PACK    (0x00)
*    P716x_DIGIN_DATA_CTRL_PACK_MODE_UNPACK  (0x10)
*/
const DWORD DATA_PACK_MODE = P716x_DIGIO_RX_DATA_CTRL_PACK_MODE_PACK;



/* NUM_PROG_ARGS - Number of Command line Arguments supported by this
* example
*/
#define NUM_PROG_ARGS   10


/* XFER_WORD_SIZE - Transfer Size; number of data samples to transfer, used
* to allocate data buffers, set DMA transfer lengths, etc.
*/
//const DWORD XFER_WORD_SIZE = 32768;    /* 32K of 32-bit samples */
const DWORD XFER_WORD_SIZE = 0x20000;
//const DWORD XFER_WORD_SIZE = 0x100000;

/* VIEW_BLK_SIZE - viewer block size in bytes.  The default is 4096. */
const DWORD VIEW_BLK_SIZE = 4096;


/* Receive Data Verification
* defaultProgArgs = "-mode", "2"
*/
#define VERIFY_RAMP_DATA      1
#define VERIFY_FIXED_DATA     2



/* Program control constants - the following constants are provided to
* alter program control for debug or evaluation purposes.
*
* DISPLAY_META_DATA - set to 1 to display meta data after each transfer.
* Default = 0.  Set to 0 to disable.
*
* DISPLAY_LAST_META_DATA - set to 1 to display meta data for last transfer.
* Default = 1.
*
* FILE_SAVE - set to 1 to save the last data buffer to a file.
* Default = 1.
*/
const DWORD  DISPLAY_CURR_META_DATA = 0;
const DWORD  DISPLAY_LAST_META_DATA = 1;
const DWORD  FILE_SAVE = 0;



/* include reg dump routines if REG_DUMP is 1 */
#if (REG_DUMP)
#include "716xregdump.h"            /* standard module registers */
#endif

/* Global variables */
EXIT_HANDLE_RESRC *ehResrc;            /* used by interrupt handler */


/* exit message table - this is a list of exit messages and their exit
* codes, used by the exitHandler() routine.
*/
static char *exitMsg[] =
{
	"program done",                                            /* 0 */

	/* program error messages */
	"Error: Pentek driver library init failed",                /* 1 */
	"Error: Digital Input module not found or failed to open", /* 2 */
	"Error: invalid command line argument",                    /* 3 */
	"Error: DMA channel failed to open",                       /* 4 */
	"Error: buffer allocation failed",                         /* 5 */
	"Error: DIGIO_RX interrupt flag active",                   /* 6 */
	"Error: invalid Signal Analyzer parameter",                /* 7 */
	"Error: cannot connect to Signal Analyzer",                /* 8 */
	"Error: semaphore creation failed",                        /* 9 */
	"Error: interrupt enable failed",                          /* 10 */
	"Error: timeout waiting for semaphore",                    /* 11 */
	"Error: error sending data to Signal Analyzer",            /* 12 */

	/* invalid error code message */
	"Error: undefined error",
	NULL
};

void getBinary(char *binVal, int val)
{
	for (int n = 0; n < 8; n++)
	{
		binVal[7 - n] = (val & 1) ? '1' : '0';
		val = (val >> 1);
	}
	binVal[8] = 0;
}

/* include Pentek High-Level Library routines */
#include "ptkhll.c"

volatile unsigned int             rxIntrCount = 0;

void dmaIntHandler(PVOID               hDev,
	LONG                dmaChannel,
	PVOID               pData,
	PTK716X_INT_RESULT *pIntResult);

CPentekReader::CPentekReader() :
m_bExiting(false),
m_bFilterFrames(true),
m_bSimulateFrames(false),
m_nCameraCapture(0),
m_sVersion(""),
m_bSaveNextTransfer(false)
{
	moduleResrc = NULL;
	libStat = PTK716X_STATUS_UNDEFINED;
	intrStat = PTK716X_STATUS_UNDEFINED;
	bufStat = PTK716X_STATUS_UNDEFINED;
	dmaHandle = NULL;


	/* Exit Handler */
	exitHdlResrc =
	{
		0,                  /* exit code */
		NULL,               /* device handle */
		NULL,               /* module resource table */
		&libStat,           /* library status */
		&intrStat,          /* interrupt status */
		&dmaHandle,         /* DMA handle */
		{
			&rxDataBuf[0],  /* Receive data buffer 1 */
			&rxDataBuf[1],  /* Receive data buffer 2 */
			&metaDataBuf[0],   /* metadata buffer 1 */
			&metaDataBuf[1]    /* metadata buffer 2 */
		},
		&ifcCurrent         /* OS resources */
	};
}

CPentekReader::~CPentekReader()
{

}

int CPentekReader::Initialize(std::vector<CameraData*> &cameras, std::string debugImageFolder)
{
	m_sDebugImageFolder = debugImageFolder;

	for (int i = 0; i < cameras.size(); i++)
	{
		m_vCameras.push_back(cameras[i]);
		m_vImageBuilders.push_back(new CImageBuilder(cameras[i], debugImageFolder));
	}

	//DWORD                 *IDTable      = (DWORD *)&(P716xValidIdTable);
	int                   *IDTable = (int *)&(P716xValidIdTable);
	DWORD                  numDevices;

	/* linked lists */
	P716x_DIGIO_RX_DMA_LLIST_DESCRIPTOR       rxDmaDescriptor[4];

	int                    status;
	DWORD                  dwStatus;
	DWORD                  linkNum = 0xF;

	/* Signal Analyzer variables and structures */
	DWORD                  useViewer = 0;  /* 0 = no; 1 = yes */
	int                    sockFd;
	int                    newSockFd;


#if (REG_DUMP)
	char                   fileName[256];
	FILE                  *regdumpfile;
#endif

	/* Program entry -------------------------------------------------------
	*
	* Initialize library access, identify board, allocate buffers, etc.
	*/

	printf("\n[%s] Entry\n", PROGRAM_ID);

	/* initialize OS-dependent Resources */
	PTKIFC_Init(&ifcCurrent);

	/* init program argument list */
	P716xCmdInitArgParams(&(progParams));

	/* apply program-specific argument values */
	PTKHLL_SetProgramOptions(&(progParams), NULL);

	/* initialize buffer pointers, tested in the exit handler */
	rxDataBuf[0].usrBuf = NULL;
	rxDataBuf[1].usrBuf = NULL;
	metaDataBuf[0].usrBuf = NULL;
	metaDataBuf[1].usrBuf = NULL;

	/* initialize the PTK716X library */
	libStat = PTK716X_LibInit();
	if (libStat != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 1;
		return (exitHdlResrc.exitCode);
	}

	/* find and open Pentek devices */
	printf("           Scanning for modules\n");
	moduleResrc = PTKHLL_DeviceFindAndOpen(IDTable, &numDevices);
	if (numDevices == 0)       /* no module found */
	{
		(exitHdlResrc.exitCode) = 2;
		return (exitHdlResrc.exitCode);
	}

	/* save module resource base address to exit handler */
	exitHdlResrc.modResrc = moduleResrc;

	/* more than one module found? */
	if (numDevices > 1)
		moduleResrc = PTKHLL_DeviceSelect(moduleResrc);

	PTKHLL_DisplayBarAddresses((char *)PROGRAM_ID, moduleResrc);


	/* assign device parameters to exit handler */
	exitHdlResrc.hDevPtr = &(moduleResrc->hDev);

	/* set useViewer parameter based on command line parameters */
	if (((progParams.viewPort) != P716x_CMDLINE_UNSUPPORTED_ARG) &&
		((progParams.viewPort) != P716x_CMDLINE_BAD_ARG))
	{
		useViewer = 1;                 /* use viewer */
	}

	/* open DMA channel and allocate buffers --------------------------- */

	/* open a DMA channel */
	dwStatus = PTK716X_DMAOpen(moduleResrc->hDev, P716x_DIGIO_RX1, &dmaHandle);
	if (dwStatus != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 4;
		return (exitHdlResrc.exitCode);
	}

	/* allocate DMA data and meta data buffers */
	bufStat = PTK716X_DMAAllocMem(dmaHandle, (progParams.xferSize << 2),
		&(rxDataBuf[0]), TRUE);
	if (bufStat != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 5;
		return (exitHdlResrc.exitCode);
	}

	/* allocate DMA data and meta data buffers */
	bufStat = PTK716X_DMAAllocMem(dmaHandle, (progParams.xferSize << 2),
		&(rxDataBuf[1]), TRUE);
	if (bufStat != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 5;
		return (exitHdlResrc.exitCode);
	}

	bufStat = PTK716X_DMAAllocMem(dmaHandle, P716x_DIGIO_RX_META_DATA_BUF_SIZE,
		&metaDataBuf[0], TRUE);
	if (bufStat != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 5;
		return (exitHdlResrc.exitCode);
	}

	bufStat = PTK716X_DMAAllocMem(dmaHandle, P716x_DIGIO_RX_META_DATA_BUF_SIZE,
		&metaDataBuf[1], TRUE);
	if (bufStat != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 5;
		return (exitHdlResrc.exitCode);
	}

	metaData[0] = (P716x_DIGIO_RX_META_DATA *)(metaDataBuf[0].usrBuf);
	metaData[1] = (P716x_DIGIO_RX_META_DATA *)(metaDataBuf[1].usrBuf);

	/* initialize the module using library routines -----------------------
	*
	* set parameters for this program; parameter defaults are set when
	* PTKHLL_DeviceFindAndOpen() is called.
	*/

	printf("[%s] Initialization\n", PROGRAM_ID);


	/* Global Parameters - the program uses internal clock A and internal
	* gate A register for the gate signal.  Both are obtained from the
	* front panel sync bus.  The module is set up as bus master, allowing
	* it to supply clock and gate to the sync bus and then use these
	* signal from the sync bus.
	*/
	moduleResrc->p716xGlobalParams.sbusParams.vcxoOutput = \
		P716x_SBUS_CTRL1_VCXO_OUT_DISABLE;


	/** DIGIO RX Parameters  **/

	moduleResrc->p716xDigioRxParams[P716x_DIGIO_RX1].dvalidPolarity = \
		(progParams.polarity) << P716x_DIGIO_RX_DATA_CTRL_DVALID_POL_SHIFT;
	moduleResrc->p716xDigioRxParams[P716x_DIGIO_RX1].suspendPolarity = \
		(progParams.polarity) << P716x_DIGIO_RX_DATA_CTRL_SUSPEND_POL_SHIFT;
	moduleResrc->p716xDigioRxParams[P716x_DIGIO_RX1].suspendEnable = \
		(progParams.suspendEnable) << P716x_DIGIO_RX_DATA_CTRL_SUSPEND_SHIFT;


	/* Configure LED drive source */
	moduleResrc->p716xGlobalParams.ledParams.digioUserLedSrc = \
		//P71610_LED_CTRL_USER_LED_SRC_CLK_DETECT;
		P71610_LED_CTRL_USER_LED_SRC_DATA_VAL_SUSPEND;



	/* DIGIO RX Channel Parameters -
	*/

	/* set DIGIO RX channel data packing mode */
	moduleResrc->p716xDigioRxParams[P716x_DIGIO_RX1].dataPackMode = DATA_PACK_MODE;


	/* apply parameter tables to registers ----------------------------- */

	P716xInitGlobalRegs(&moduleResrc->p716xGlobalParams,
		&moduleResrc->p716xRegs);

	status = P716xInitDigioRxRegs(&moduleResrc->p716xDigioRxParams[P716x_DIGIO_RX1],
		&moduleResrc->p716xRegs,
		P716x_DIGIO_RX1);

	/* enable Gate mode in the Gate Trigger Control register */
	P716xSetDigioRxGateTrigCtrlTriggerMode(
		moduleResrc->p716xRegs.digioRxRegs[P716x_DIGIO_RX1].gateTrigControl,
		P716x_DIGIO_RX_GATE_TRIG_CTRL_TRIG_MODE_GATE);

	/* DIGIO_RX Input Delay Tap Control register */
	//int nDelayPicoSec = 2325; // Max is 31 * 75 = 2325 (only first 5 bits get in)
	int nDelayPicoSec = 0; // No delay gives clean images
	P716xSetDigioRxIDelayTapCtrlTapDelay(
		moduleResrc->p716xRegs.digioRxRegs[P716x_DIGIO_RX1].inDelayTapControl,
		nDelayPicoSec);

	// Get the version
	char version[50];
	unsigned int revVal, dateVal;
	P716x_REG_DUMP(moduleResrc->p716xRegs.FPGACodeDate, dateVal);
	P716x_REG_DUMP(moduleResrc->p716xRegs.FPGACodeRevision, revVal);

	sprintf_s(version, "%06x v%d", dateVal, revVal);
	m_sVersion = version;

	/* DMA setup ------------------------------------------------------- */

	/* reset the DMA linked list engine and FIFO */
	P716xDigioRxDmaReset(&moduleResrc->p716xRegs, P716x_DIGIO_RX1);

	/* this program uses 4 descriptors.  The first transfers data to the
	* first data buffer.  The second transfers meta data for the first
	* data transfer.  The third transfers data to the second data buffer.
	* the last transfers meta data associated with the second data
	* transfer.
	*/

	/* descriptor 0 - data transfer descriptor for 1st data buffer */
	rxDmaDescriptor[0].linkCtrlWord =
		(P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_DEFAULT |
		P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_MODE_AUTO |
		(1 << P716x_DIGIO_RX_DMA_CWORD_NEXT_LINK_ADDR_OFFSET) |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_MODE_STORE |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_LINK_RX_DATA |
		P716x_DIGIO_RX_DMA_CWORD_START_MODE_MANUAL |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_MODE_NORMAL |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_INTR_DISABLE |
		P716x_DIGIO_RX_DMA_CWORD_CHAIN_END_INTR_DISABLE |
		P716x_DIGIO_RX_DMA_CWORD_END_OF_CHAIN_DISABLE);
	rxDmaDescriptor[0].xferLength = (progParams.xferSize << 2);
	P716xSetDigioRxDmaLListDescriptorAddress(rxDataBuf[0].kernBuf,
		&rxDmaDescriptor[0].mswAddress,
		&rxDmaDescriptor[0].lswAddress);

	/* descriptor 1 - meta data transfer descriptor for 1st data transfer */
	rxDmaDescriptor[1].linkCtrlWord =
		(P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_DEFAULT |
		P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_MODE_AUTO |
		(2 << P716x_DIGIO_RX_DMA_CWORD_NEXT_LINK_ADDR_OFFSET) |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_MODE_NO_STORE |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_LINK_META_DATA |
		P716x_DIGIO_RX_DMA_CWORD_START_MODE_AUTO |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_MODE_NORMAL |
		P716x_DIGIO_RX_DMA_CWORD_CHAIN_END_MODE_NORMAL |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_INTR_ENABLE);
	rxDmaDescriptor[1].xferLength = P716x_DIGIO_RX_META_DATA_BUF_SIZE;
	P716xSetDigioRxDmaLListDescriptorAddress(metaDataBuf[0].kernBuf,
		&rxDmaDescriptor[1].mswAddress,
		&rxDmaDescriptor[1].lswAddress);

	/* descriptor 2 - data transfer descriptor for 2nd data buffer */
	rxDmaDescriptor[2].linkCtrlWord =
		(P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_DEFAULT |
		P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_MODE_AUTO |
		(3 << P716x_DIGIO_RX_DMA_CWORD_NEXT_LINK_ADDR_OFFSET) |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_MODE_STORE |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_LINK_RX_DATA |
		P716x_DIGIO_RX_DMA_CWORD_START_MODE_AUTO |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_MODE_NORMAL |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_INTR_DISABLE |
		P716x_DIGIO_RX_DMA_CWORD_CHAIN_END_INTR_DISABLE |
		P716x_DIGIO_RX_DMA_CWORD_END_OF_CHAIN_DISABLE);
	rxDmaDescriptor[2].xferLength = (progParams.xferSize << 2);
	P716xSetDigioRxDmaLListDescriptorAddress(rxDataBuf[1].kernBuf,
		&rxDmaDescriptor[2].mswAddress,
		&rxDmaDescriptor[2].lswAddress);

	/* descriptor 3 - meta data transfer descriptor for 2nd data transfer */
	rxDmaDescriptor[3].linkCtrlWord =
		(P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_DEFAULT |
		P716x_DIGIO_RX_DMA_CWORD_PAYLOAD_SIZE_MODE_AUTO |
		(0 << P716x_DIGIO_RX_DMA_CWORD_NEXT_LINK_ADDR_OFFSET) |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_MODE_NO_STORE |
		P716x_DIGIO_RX_DMA_CWORD_META_DATA_LINK_META_DATA |
		P716x_DIGIO_RX_DMA_CWORD_START_MODE_AUTO |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_MODE_NORMAL |
		P716x_DIGIO_RX_DMA_CWORD_CHAIN_END_MODE_NORMAL |
		P716x_DIGIO_RX_DMA_CWORD_LINK_END_INTR_ENABLE);
	rxDmaDescriptor[3].xferLength = P716x_DIGIO_RX_META_DATA_BUF_SIZE;
	P716xSetDigioRxDmaLListDescriptorAddress(metaDataBuf[1].kernBuf,
		&rxDmaDescriptor[3].mswAddress,
		&rxDmaDescriptor[3].lswAddress);



	/* set descriptor memory to descriptors parameters */
	for (int cntr = 0; cntr < 4; cntr++)
	{
		P716xInitDigioRxDmaLListDescriptor(&rxDmaDescriptor[cntr],
			&moduleResrc->p716xRegs, P716x_DIGIO_RX1, cntr);
	}

	//puts("           Waiting for TX ready, press any key to continue...\n");
	//getchar();

	/* Flush the DIGIO RX Input FIFOs */
	P716xDigioRxFifoFlush(&moduleResrc->p716xRegs, P716x_DIGIO_RX1);

	/* reset the DMA linked list engine */
	P716xDigioRxDmaReset(&moduleResrc->p716xRegs, P716x_DIGIO_RX1);



	/* Create a DMA Complete semaphore for this DMA channel */
	if ((PTKIFC_SemaphoreCreate(&ifcCurrent, P716x_DIGIO_RX1)) < 0)
	{
		exitHdlResrc.exitCode = 9;
		return (exitHdlResrc.exitCode);
	}

	/* Interrupt Enable */
	intrStat = PTK716X_intEnable(
		moduleResrc->hDev,
		(PTK71610_PCIE_INTR_INPUT_ACQ_MOD),
		P716x_DIGIO_RX_INTR_LINK_END,
		(PVOID)(exitHdlResrc.ifcArgs),
		dmaIntHandler);
	if (intrStat != PTK716X_STATUS_OK)
	{
		exitHdlResrc.exitCode = 10;
		return (exitHdlResrc.exitCode);
	}


#if (REG_DUMP)
	sprintf(fileName, "%s%d_regdump.txt", PROGRAM_ID,
		P716x_DIGIO_RX1);

	regdumpfile = fopen(fileName, "w");
	if (regdumpfile != NULL)
	{
		fprintf(regdumpfile, "[%s] Debug Register Dump\n", fileName);
		fprintf(regdumpfile, "    Before 1st DMA:\n");
		P716xBoardIdRegDump(&(moduleResrc->p716xRegs), regdumpfile);
		P716xPcieRegDump(&(moduleResrc->p716xRegs), regdumpfile);
		P716xGlobalRegDump(&(moduleResrc->p716xRegs), regdumpfile);
		P716xDigioRxRegDump(&(moduleResrc->p716xRegs), P716x_DIGIO_RX1, regdumpfile);
		P716xDigioRxDmaLListDescriptorDump(&(moduleResrc->p716xRegs), P716x_DIGIO_RX1,
			0, 3, regdumpfile);

		fclose(regdumpfile);
	}
#endif

	/* Main loop -------------------------------------------------------- */

	LOG4CXX_INFO(logger, "Initialize");

	// Create the processing buffers
	unsigned int	rcvCount = progParams.xferSize;
	for (int i = 0; i < NUM_PROC_BUFFERS; i++)
	{
		DWORD *ptr = new DWORD[rcvCount];
		AddFreeBuffer(ptr);
	}

	/* Enable Gate */
	P716xSetGateGenState((&moduleResrc->p716xRegs)->gateAGenerate, P716x_GATE_ACTIVE);

	/* Start the DMA */
	P716xDigioRxDmaStart(&moduleResrc->p716xRegs, P716x_DIGIO_RX1);

	

	// Create the capture thread
	DWORD dwThreadID;
	m_hCaptureThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CPentekReader::CaptureThread,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier
	SetThreadPriority(m_hCaptureThread, THREAD_PRIORITY_HIGHEST);

#if !PROG_DEBUG
	// Create the processing thread
	m_hProcessingThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CPentekReader::ProcessingThread,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier
	SetThreadPriority(m_hProcessingThread, THREAD_PRIORITY_HIGHEST);
#endif

	return 0;
}

void CPentekReader::Close()
{
	m_bExiting = true;
	WaitForSingleObject(m_hCaptureThread, INFINITE);
	WaitForSingleObject(m_hProcessingThread, INFINITE);

	try
	{
		/* Disable Gate */
		if (moduleResrc)
		{
			P716xSetGateGenState((&moduleResrc->p716xRegs)->gateAGenerate, P716x_GATE_INACTIVE);
		}

		DWORD *ptr;
		do
		{
			ptr = GetFreeBuffer();
			delete[] ptr;
		} while (ptr != NULL);

		do
		{
			ptr = GetFullBuffer();
			delete[] ptr;
		} while (ptr != NULL);
	}
	catch (...)
	{
	}

	/* clean up and exit */
	//exitHdlResrc.exitCode = 0;
	exitHandler(&(exitHdlResrc));
}

void CPentekReader::ResetAllImages()
{
	LOG4CXX_INFO(logger, "Resetting all incoming images");

	for (int i = 0; i < m_vImageBuilders.size(); i++)
	{
		m_vImageBuilders[i]->Reset();
	}
}

DWORD *CPentekReader::GetFreeBuffer()
{
	DWORD *pBuffer = NULL;

#if SPEED_TEST
	bool succeed = m_FreeBufferQueue.try_dequeue(pBuffer);
	if (!succeed)
		pBuffer = NULL;
#else
	m_FreeCriticalSection.Lock();

	if (!m_vFreeBuffers.empty())
	{
		pBuffer = m_vFreeBuffers.front();
		m_vFreeBuffers.pop();
	}

	m_FreeCriticalSection.Unlock();
#endif

	return pBuffer;
}

DWORD *CPentekReader::GetFullBuffer()
{
	DWORD *pBuffer = NULL;

#if SPEED_TEST
	bool succeed = m_FullBufferQueue.try_dequeue(pBuffer);
	if (!succeed)
		pBuffer = NULL;
#else
	m_FullCriticalSection.Lock();

	if (!m_vFullBuffers.empty())
	{
		pBuffer = m_vFullBuffers.front();
		m_vFullBuffers.pop();
	}

	m_FullCriticalSection.Unlock();
#endif
	return pBuffer;
}

void CPentekReader::AddFreeBuffer(DWORD *ptr)
{
#if SPEED_TEST
	m_FreeBufferQueue.enqueue(ptr);
#else
	m_FreeCriticalSection.Lock();

	m_vFreeBuffers.push(ptr);

	m_FreeCriticalSection.Unlock();
#endif
}

void CPentekReader::AddFullBuffer(DWORD *ptr)
{
#if SPEED_TEST
	m_FullBufferQueue.enqueue(ptr);
#else
	m_FullCriticalSection.Lock();

	m_vFullBuffers.push(ptr);

	m_FullCriticalSection.Unlock();
#endif
}

void CPentekReader::SaveFullBuffers()
{
	m_FullCriticalSection.Lock();

	unsigned int rcvCount = progParams.xferSize;

	int index = 0;
	DWORD *buffer = GetFullBuffer();
	while (buffer!= NULL)
	{
		char fname[128];
		sprintf(fname, "%s\\FullBuffer_%02d.bin", m_sDebugImageFolder.c_str(), index);
		index++;
#if 1
		std::ofstream file;
		file.open(fname, std::ios::out | std::ios::binary);
		file.write((const char *)buffer, rcvCount * 4);
		file.close();
#else		
		WriteBufToFile((DWORD *)m_vFullBuffers.front(),
			rcvCount,
			fname);
#endif
		buffer = GetFullBuffer();
	}

	m_FullCriticalSection.Unlock();

}

void CPentekReader::LoadSimulationData()
{
	m_FullCriticalSection.Lock();

	m_bSimulateFrames = true;

	for (int index = 0; index < NUM_PROC_BUFFERS; index++)
	{
		unsigned int rcvCount = progParams.xferSize;
		DWORD *pBuffer = new DWORD[rcvCount];
		
		char fname[128];
		sprintf(fname, "%s\\FullBuffer_%02d.bin", m_sDebugImageFolder.c_str(), index);

		std::ifstream file;
		file.open(fname, std::ios::in | std::ios::binary);
		file.read((char *)pBuffer, rcvCount * 4);
		file.close();

		AddFullBuffer(pBuffer);
	}

	m_FullCriticalSection.Unlock();

	// Create the processing thread
	DWORD dwThreadID;
	m_hProcessingThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CPentekReader::ProcessingThread,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier
	SetThreadPriority(m_hProcessingThread, THREAD_PRIORITY_NORMAL);
}

void CPentekReader::SaveNextTransfer()
{
	m_bSaveNextTransfer = true; 
	for (int i = 0; i < m_vImageBuilders.size(); i++)
	{
		m_vImageBuilders[i]->SaveNextTransfer();
	}
}

bool CPentekReader::IsSaveNextTransferOn()
{
	bool bOn = m_bSaveNextTransfer;
	m_bSaveNextTransfer = false;

	return bOn;
}

int CPentekReader::WriteBufToFile(DWORD *buf,
	DWORD  size,
	char  *progId)
{
	char  outfile[180];
	char  filepath[60];
	char  filename[60];
	char  typeStr[10];
	DWORD bytesSaved;

	sprintf(outfile, "%s_DataTransfer.txt", progId);

	/* write captured data */
	int count = 0;
	FILE *file;
	if ((file = fopen(outfile, "w")) == NULL)
	{
		printf("[P716xCmd_Buff2File] Output file, %s, failed to open\n",
			outfile);
		return(-1);
	}

	FILE *cam_file[5];
	for (int i = 0; i < 5; i++)
	{
		sprintf(outfile, "%s_CameraData_%d.txt", progId, i);
		if ((cam_file[i] = fopen(outfile, "w")) == NULL)
		{
			printf("[P716xCmd_Buff2File] Output file, %s, failed to open\n",
				outfile);
			return(-1);
		}
	}

	LOG4CXX_INFO(logger, "Opening file for write: " << outfile);

	std::stringstream ss;
	int dataValLow;
	int dataValHigh;
	int loadBit = 0x4;
	int vsyncBit = 0x10;
	int hsyncBit = 0x20;
	BOOL isLoad;

	while (size--)
	{
		unsigned long *sample = (buf + count);
		int val = (*sample & 0xFFF);
		int vsync = ((*sample & 0x1000) == 0x1000) ? 1 : 0;
		int hsync = ((*sample & 0x2000) == 0x2000) ? 1 : 0;
		int cam = ((*sample & 0x70000) >> 16);
		fprintf(file, "Sample[%08x], Cam[%d], V%d, H%d, Val[%d]\n", *sample, cam, vsync, hsync, val);

		if ((cam >= 0) && (cam < 5))
		{
			fprintf(cam_file[cam], "#%d, Sample[%08x], Cam[%d], V%d, H%d, Val[%d]\n", count, *sample, cam, vsync, hsync, val);
		}
		
		count++;
	}  /* end while( size-- ) */

	bytesSaved = count << 2;
	fclose(file);

	for (int i = 0; i < 5; i++)
	{
		fclose(cam_file[i]);
	}

	return (0);
}

void CPentekReader::ProcessAllCameraBuffer(unsigned long *recvImage, unsigned int bufferLength)
{
	//LOG4CXX_DEBUG(logger, "Start processing buffer");

	const int vsyncBit = 0x1000;
	const int hsyncBit = 0x2000;
	bool isHSync;
	bool isVSync;

	int bufferNum = NUM_PROC_BUFFERS - (m_vFullBuffers.size() + 1);
	
	bool bSaveData = IsSaveNextTransferOn();
	//bSaveData = true;
	if (0)//bSaveData)
	{
		char fname[128];
		sprintf(fname, "%s\\FullBuffer_%02d", m_sDebugImageFolder.c_str(), bufferNum);
		WriteBufToFile((DWORD *)recvImage, bufferLength, fname);
	}

	for (unsigned int i = 0; i < bufferLength; i++)
	{
		unsigned long val = recvImage[i];

		isVSync = ((val & vsyncBit) == vsyncBit);
		isHSync = ((val & hsyncBit) == hsyncBit);
		int cam = ((val & 0x70000) >> 16);
		ushort pixVal16 = (val & 0xfff);

		m_vImageBuilders[cam]->AddPixel(pixVal16, isVSync, isHSync, m_bFilterFrames);
		
	}  /* end for( ) */

	//LOG4CXX_DEBUG(logger, "Finished processing buffer");
}


UINT CPentekReader::CaptureThread(LPVOID param)
{
	CPentekReader *pReader = (CPentekReader *)param;

	LOG4CXX_INFO(logger, "Started CaptureThread");

	DWORD                  cntr;
	int                    status;
	unsigned int           curBufIndex = 1, errCount;
	unsigned int           rxLast = 0;
	P716x_DIGIO_RX_META_DATA_PARAMS  metaDataParams;
	unsigned int		   rcvCount = pReader->GetProgParams()->xferSize;
	/* main loop */
	do
	{
		/* When DMA has completed, it will post the semphore */
		status = PTKIFC_SemaphoreWait(pReader->GetIFCArgs(), P716x_DIGIO_RX1,
			IFC_WAIT_STATE_MILSEC(5000));
		if (status != 0)
		{
			LOG4CXX_ERROR(logger, "Timeout waiting for DMA complete semaphore. Interrupt count is " << rxIntrCount);

			pReader->GetExitHandleResrc()->exitCode = 11;
			
			continue;
		}

		DWORD *pBuffer = pReader->GetFreeBuffer();
		if (pBuffer != NULL)
		{
			/* Get transfer count */
			if ((rxIntrCount % 2) != 0)
			{
				//if (curBufIndex == 0)
				//	LOG4CXX_ERROR(logger, "Duplicate block detected!");

				curBufIndex = 0;
			}
			else
			{
				//if (curBufIndex == 1)
				//	LOG4CXX_ERROR(logger, "Duplicate block detected!");

				curBufIndex = 1;
			}

			/* Find out how many 32-bit words are sent by transmitter */
			rcvCount = (P716xGetDigioRxMetaDataParam(pReader->GetMetaData(curBufIndex),
				P716x_DIGIO_RX_META_DATA_VALID_WORD_CNT)) >> 1;

			DWORD *bufPtr = (DWORD *)(pReader->GetRxDataBuffer(curBufIndex)->usrBuf);

			//Copy into recv Image (process when full)
			memcpy(pBuffer, bufPtr, sizeof(DWORD)*rcvCount);
			pReader->AddFullBuffer(pBuffer);
		}
		else
		{
#if PROG_DEBUG
			LOG4CXX_ERROR(logger, "Saving all buffers");
			pReader->SaveFullBuffers();
			return 0;
#else
			LOG4CXX_ERROR(logger, "No Free Buffer Available!");
#endif
		}
		
		/* Start the DMA */
		P716xDigioRxDmaStart(&(pReader->GetModuleResrc()->p716xRegs), P716x_DIGIO_RX1);

	} while (!pReader->IsExiting() && !PTKIFC_Kbhit());

	/* display meta data if desired */
	if (DISPLAY_LAST_META_DATA)
	{
		puts("[digio_rx] Meta data for last transfer:");
		P716xGetDigioRxMetaDataParams(pReader->GetMetaData(0), &metaDataParams);
		displayMetaData(&metaDataParams, stdout);
		puts("");
		P716xGetDigioRxMetaDataParams(pReader->GetMetaData(1), &metaDataParams);
		displayMetaData(&metaDataParams, stdout);
	}

	/* save data buffers if desired */
	if (FILE_SAVE)
	{
		P716x_CMDLINE_ARGS *progParams = pReader->GetProgParams();
		puts("         Saving last captured data buffer");
		/*status = PTKHLL_WriteBufToFile (rxDataBuf[1].usrBuf, (progParams.xferSize << 2),
		moduleResrc->moduleId,
		PROGRAM_ID, P716x_DIGIO_RX1,
		progParams.devType,
		progParams.datFormat);*/
		status = pReader->WriteBufToFile((DWORD *)pReader->GetRxDataBuffer(0)->usrBuf,
			(progParams->xferSize << 2),
			PROGRAM_ID);
		if (status != 0)
			puts("         Warning: write to save file failed");
		else
			PTKHLL_ScriptUsage((progParams->xferSize << 2), PROGRAM_ID, progParams->devType,
			P716x_DIGIO_RX1, progParams->datFormat);
	}


#if (REG_DUMP)
	char                   fileName[256];
	FILE                  *regdumpfile;
	sprintf(fileName, "PentekReg.txt");

	regdumpfile = fopen(fileName, "a");
	if (regdumpfile != NULL)
	{
		fprintf(regdumpfile, "[%s] Debug Register Dump\n", fileName);
		fprintf(regdumpfile, "    After Gate is disabled:\n");

		P716x_REG_ADDR *p716xRegs = &(pReader->GetModuleResrc()->p716xRegs);
		P716xBoardIdRegDump(p716xRegs, regdumpfile);
		P716xPcieRegDump(p716xRegs, regdumpfile);
		P716xGlobalRegDump(p716xRegs, regdumpfile);
		P716xDigioRxRegDump(p716xRegs, P716x_DIGIO_RX1, regdumpfile);
		P716xDigioRxDmaLListDescriptorDump(p716xRegs, P716x_DIGIO_RX1,
			0, 3, regdumpfile);

		fclose(regdumpfile);
	}
#endif

	LOG4CXX_INFO(logger, "Finishing CaptureThread");

	return 0;
}

UINT CPentekReader::ProcessingThread(LPVOID param)
{
	CPentekReader *pReader = (CPentekReader *)param;

	LOG4CXX_INFO(logger, "Started ProcessingThread");

	unsigned int		   rcvCount = pReader->GetProgParams()->xferSize;

	/* main loop */
	do
	{
		DWORD *pBuffer = pReader->GetFullBuffer();
		if (pBuffer == NULL)
		{
			//LOG4CXX_ERROR(logger, "No Full Buffer Available!");
			//pReader->ResetAllImages();
			
			continue;
		}

		pReader->ProcessAllCameraBuffer(pBuffer, rcvCount);

		if (pReader->IsSimulationEnabled())
		{
			pReader->AddFullBuffer(pBuffer);
		}
		else
		{
			pReader->AddFreeBuffer(pBuffer);
		}

	} while (!pReader->IsExiting() && !PTKIFC_Kbhit());

	LOG4CXX_INFO(logger, "Finishing ProcessingThread");

	return 0;
}
/************************************************************************
Function: dmaIntHandler

Description: This routine serves as the User Mode interrupt handler
for this example.

Inputs:      hDev        - 716x Device Handle
dmaChannel  - DMA channel generating the interrupt(0-3)
pData       - Pointer to user defined data
pIntResults - Pointer to the interrupt results structure

Returns:     none

Notes:       DMA interrupts are cleared by the Kernel Device Driver.
DMA interrupts are enabled when this routine is executed.
************************************************************************/
static void dmaIntHandler(PVOID               hDev,
	LONG                dmaChannel,
	PVOID               pData,
	PTK716X_INT_RESULT *pIntResult)
{

	IFC_ARGS *ifcArgs = (IFC_ARGS *)pData;

	/* Confirm interrupt */
	if (((pIntResult->intFlag) & P716x_DIGIO_RX_INTR_LINK_END) ==
		P716x_DIGIO_RX_INTR_LINK_END)
	{
		PTKIFC_SemaphorePost(ifcArgs, (int)dmaChannel);
		rxIntrCount++;
	}
	else
	{
		LOG4CXX_ERROR(CPentekReader::logger, "Received unexpected interrupt 0x" << std::hex << pIntResult->intFlag);
	}
}


/**************************************************************************
Function:     PTKHLL_DeviceInit()

Description:  This routine initializes the module address tables, resets
the module and sets all module parameters to default values.

Inputs:       pointer to module resource table
Return:       0 = success
1 = module does not support example program
2 = found 71610 module not an Input board
**************************************************************************/
static int PTKHLL_DeviceInit(MODULE_RESRC *moduleResrc)
{
	DWORD cntr;


	/* get actual module ID */
	moduleResrc->moduleId = P716xGetFPGACodeTypeFPGAModuleId(\
		(volatile unsigned int *)(moduleResrc->BAR0Base +
		P716x_FPGA_CODE_TYPE));

	/* initialize module; verify it support this program */
	if (moduleResrc->moduleId == P71610_MODULE_ID)
	{

		/* as this is an Input example program, verify this module
		* is a DIGIO RX module; if not, do not continue with
		* the initialization and omit it from the list.
		*/
		if ((P716xGetDaughterBoardInterfaceType(\
			(volatile unsigned int *)(moduleResrc->BAR0Base +
			P716x_DAUGHTER_BOARD_ID)))
			!= P716x_DAUGHTER_BOARD_ID_INTERFACE_TYPE_INPUT)
		{
			return (2);
		}
	}
	else  /* Omit all other modules */
	{
		return (1);
	}


	/* initialize register address and board resource tables */
	P716xInitRegAddr(moduleResrc->BAR0Base,
		&moduleResrc->p716xRegs,
		&moduleResrc->p716xBrdResrc,
		moduleResrc->moduleId);


	/* reset all registers to default values */
	P716xResetRegs(&moduleResrc->p716xRegs);

	/* load parameter tables with default values */
	P716xSetGlobalDefaults(&moduleResrc->p716xBrdResrc,
		&moduleResrc->p716xGlobalParams);

	for (cntr = 0; cntr < moduleResrc->p716xBrdResrc.numDIGIORX; cntr++)
		P716xSetDigioRxDefaults(&moduleResrc->p716xBrdResrc,
		&moduleResrc->p716xDigioRxParams[cntr]);

	return (0);
}


/**************************************************************************
Function: PTKHLL_SetProgramOptions()

Description: This routine sets the selected elemenets in the Command Line
Argument structure with program default values.
Note that these are the only elements supported for
this example program.

Inputs:      argParams - pointer to the P716x_CMDLINE_ARGS command line
argument structure.
brdResrc  - pointer to the P716x_BOARD_RESOURCE structure.
This structure must be initialized before this
function is called.

Returns:     none
**************************************************************************/
static void PTKHLL_SetProgramOptions(P716x_CMDLINE_ARGS   *argParams,
	P716x_BOARD_RESOURCE *brdResrc)
{
	argParams->devType = P716x_DIGIO_RX;
	argParams->xferSize = XFER_WORD_SIZE;
	argParams->rateDiv = 1;
	argParams->loop = LOOP_COUNT;
	argParams->mode = VERIFY_FIXED_DATA;
	argParams->viewPort = P716x_CMDLINE_BAD_ARG;    /* An invalid P716x_DIGIO_RX1 # */
	strcpy(&(argParams->viewServAddr[0]), "localhost");
	argParams->datFormat = P716x_CMDLINE_FILE_FORMAT_ASCII;// IFC_DATA_FORMAT; AEG Saving txt file
	argParams->suspendEnable = P716x_CMDLINE_SUSPEND_ENABLE;
	argParams->polarity = P716x_CMDLINE_POLARITY_POS;

}


/**************************************************************************
Function: PTKHLL_ViewerSetControlWord()

Description: This routine sets the selected elemenets in the Signal
Analyzer Control Argument structure with program default
values.
Inputs:      boardId      - module ID number, ie: 71660
viewCtrlWord - pointer to the P716x_VIEW_CONTROL
Signal View Control structure.
argParams    - pointer to the P716x_CMDLINE_ARGS
Command line Argument structure.
Returns:     none
**************************************************************************/
static void PTKHLL_ViewerSetControlWord(DWORD               moduleId,
	P716x_VIEW_CONTROL *viewCtrlWord,
	P716x_CMDLINE_ARGS *progParams)
{
	viewCtrlWord->board = moduleId;
	viewCtrlWord->clock = (float)((progParams->clockFreq) / 1.0e6); /*float in Mhz */
	viewCtrlWord->decimation = progParams->rateDiv;
	viewCtrlWord->packingMode = 2;   /* 2=16-bit time packed */
	viewCtrlWord->channelType = 0; // progParams->devType;   /* ADC=0, DDC=1 */
	viewCtrlWord->centerFreq = 0.0;  /* float in Mhz */
	viewCtrlWord->voltageLevel = 2;  /* 0=4dBm, 1=10dBm 2=8dBm*/
	viewCtrlWord->blockSize = VIEW_BLK_SIZE;  /* in bytes */
	viewCtrlWord->realComplex = 0;      /* 0 = complex */
	viewCtrlWord->adcResolution = 16;   /* 16 bits for the ADC */
}

string CPentekReader::GetErrorMessage(int index)
{
	int   msgCnt;

	/* count messages in exitMsg */
	msgCnt = 0;
	while (exitMsg[msgCnt] != NULL)
		msgCnt++;
	msgCnt--;


	/* display message */
	if (index >= msgCnt)
		index = msgCnt;

	return exitMsg[index];
}

/**************************************************************************
Function: exitHandler

Description:  This Routine serves as an program Exit Handler and a Signal
Handler.  It it to output an appropriate message at program
exit and cleans up before program exit.
Inputs:       ehResrc - pointer to exit handler resource structure
Return:       exit code from ehResrc
**************************************************************************/
int CPentekReader::exitHandler(EXIT_HANDLE_RESRC *ehResrc)
{
	LOG4CXX_INFO(logger, "Exit Code = " << GetErrorMessage(ehResrc->exitCode));

	/* check if library was initialized */
	if (*(ehResrc->libStatPtr) == PTK716X_STATUS_OK)
	{
		/* check if device was found */
		if (ehResrc->hDevPtr != NULL)
		{
			/* abort DMA */
			P716xDigioRxDmaAbort(&(ehResrc->modResrc->p716xRegs), P716x_DIGIO_RX1);

			/* disable DMA interrupt */
			if (*(ehResrc->intrStatPtr) == PTK716X_STATUS_OK)
			{
				PTK716X_intDisable(
					*(ehResrc->hDevPtr),
					(PTK71610_PCIE_INTR_INPUT_ACQ_MOD << P716x_DIGIO_RX1),
					P716x_DIGIO_RX_INTR_LINK_END);
			}

			BaseboardDelayuS(1000);

			/* free buffers */
			if ((*(ehResrc->dmaDataBuf[0])).usrBuf != NULL)
				PTK716X_DMAFreeMem(*(ehResrc->dmaHandlePtr),
				(ehResrc->dmaDataBuf[0]));
			if ((*(ehResrc->dmaDataBuf[1])).usrBuf != NULL)
				PTK716X_DMAFreeMem(*(ehResrc->dmaHandlePtr),
				(ehResrc->dmaDataBuf[1]));
			if ((*(ehResrc->dmaDataBuf[2])).usrBuf != NULL)
				PTK716X_DMAFreeMem(*(ehResrc->dmaHandlePtr),
				(ehResrc->dmaDataBuf[2]));

			/* close DMA channel */
			if (*(ehResrc->dmaHandlePtr) != NULL)
				PTK716X_DMAClose(*(ehResrc->hDevPtr),
				*(ehResrc->dmaHandlePtr));
		}

		/* close all open Pentek devices */
		PTKHLL_DeviceClose(ehResrc->modResrc);

		/* library was initialized, uninit now */
		PTK716X_LibUninit();

		/* Release all OS-dependent resources */
		PTKIFC_UnInit((ehResrc->ifcArgs));
	}

	return ((ehResrc->exitCode));
}

/**************************************************************************
Function: displayMetaData()

Description:  Routine to display meta data to console output.
Inputs:       P716x_META_DATA_PARAMS *metaDataParams
FILE *outPtr

Return:       none
**************************************************************************/
void displayMetaData(P716x_DIGIO_RX_META_DATA_PARAMS *metaData,
	FILE                      *outPtr)
{
	printf("         Gate Number               = %d\n",
		metaData->gateNumber);
	printf("         Trig Number               = %d\n",
		metaData->trigNumber);
	printf("         Valid Word count          = %d\n",
		metaData->validWordCount);
	printf("         Channel number            = %d\n",
		metaData->chanNumber);
	printf("         FIFO overflow             = %d\n",
		metaData->fifoOverflow);
	printf("         Link number               = %d\n",
		metaData->linkNumber);
	printf("         Packet count              = %d\n",
		metaData->packetCount);
	printf("         Destination address LSB   = 0x%x\n",
		metaData->destAddrLsb);
	printf("         Destination address MSB   = 0x%x\n",
		metaData->destAddrMsb);
}


