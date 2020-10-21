#include "StdAfx.h"

#include "CefClientApp.h"
#include "CefClientHandler.h"
#include "V8Handler.h"

#include "include/wrapper/cef_helpers.h"

void CefClientApp::OnContextInitialized()
{
	CEF_REQUIRE_UI_THREAD();

	// CefClientHandler implements browser-level callbacks.

	m_cefHandler = new CefClientHandler();
}


BOOL CefClientApp::CreateBrowser(HWND hWnd, CRect rect, LPCTSTR pszURL)
{
	// settings
	CefBrowserSettings settings;
	CefWindowInfo info;

	// set browser as child
	info.SetAsChild(hWnd, rect);

	// create browser window
	return CefBrowserHost::CreateBrowser(info, m_cefHandler, pszURL, settings, NULL, NULL);
}

void CefClientApp::OnWebKitInitialized()
{
    // Create the renderer-side router for query handling.
    CefMessageRouterConfig config;
    m_message_router = CefMessageRouterRendererSide::Create(config);
}

void CefClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) 
{
    m_message_router->OnContextCreated(browser, frame, context);
}

void CefClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    m_message_router->OnContextReleased(browser, frame, context);
}

bool CefClientApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    return m_message_router->OnProcessMessageReceived(browser, frame, source_process, message);
}