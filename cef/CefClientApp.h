#pragma once

#include "CefClientHandler.h"

#include "include/cef_app.h"
#include "include/wrapper/cef_message_router.h"

class CefClientApp : public CefApp,
	public CefBrowserProcessHandler,
	public CefRenderProcessHandler
{
public:

	// CefApp methods:
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE { return this; }

	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE { return this; } 

	// CefBrowserProcessHandler methods:
	virtual void OnContextInitialized() OVERRIDE;

    // CefRenderProcessHandler methods:
    virtual void OnWebKitInitialized() OVERRIDE;

    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;

    virtual void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) OVERRIDE;


	BOOL CreateBrowser(HWND hWnd, CRect rect, LPCTSTR pszURL);

public:
	CefRefPtr<CefClientHandler> m_cefHandler;

private:
	// Handles the renderer side of query routing.
	CefRefPtr<CefMessageRouterRendererSide> m_message_router;


	// Include the default reference counting implementation.
	IMPLEMENT_REFCOUNTING(CefClientApp);
};