// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__1D4DE893_F600_42D8_985D_0B2AD98EBA4B__INCLUDED_)
#define AFX_STDAFX_H__1D4DE893_F600_42D8_985D_0B2AD98EBA4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0601		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0601	// Change this to the appropriate value to target other versions of Windows.
#endif	

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _ATL_FREE_THREADED

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define ROUND(x) ( (INT)( ((x)<0) ? ((x)-.5) : ((x)+.5) ) )
#define RoundL(x) ( (LL)( ((x)<0) ? ((x)-.5) : ((x)+.5) ) )

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1D4DE893_F600_42D8_985D_0B2AD98EBA4B__INCLUDED_)
