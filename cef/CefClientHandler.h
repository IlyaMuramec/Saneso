#pragma once

#include "include/cef_client.h"
#include "include/wrapper/cef_message_router.h"

#include "../StatusClasses.h"

class CefClientHandler : public CefClient,
    public CefDownloadHandler,
    public CefDisplayHandler,
    public CefLifeSpanHandler,
    public CefRequestHandler,
    public CefLoadHandler {
public:
    explicit CefClientHandler();
    virtual ~CefClientHandler();

    // Provide access to the single global instance of this object.
    static CefClientHandler* GetInstance();

    // CefClient methods:
    virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE { return this; }
    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) OVERRIDE;

    // CefDisplayHandler methods:
    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) OVERRIDE;

    // CefLifeSpanHandler methods:
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

    // CefLoadHandler methods:
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        ErrorCode errorCode,
        const CefString& errorText,
        const CefString& failedUrl) OVERRIDE;
    
    // CefDownloadHandler methods
    virtual void OnBeforeDownload(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item, 
        const CefString& suggested_name, 
        CefRefPtr<CefBeforeDownloadCallback> callback) OVERRIDE;

    virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, 
        CefRefPtr<CefDownloadItem> download_item, 
        CefRefPtr<CefDownloadItemCallback> callback) OVERRIDE;

    // CefRequestHandler methods:
    virtual  bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool user_gesture,
        bool is_redirect) OVERRIDE;

    virtual void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status) OVERRIDE;

    // Request that existing browser window close.
    void CloseBrowser(bool force_close);

    bool IsClosing() const { return m_is_closing; }

    void SendCalibrationStatus(CalStatuses cal_statuses);

private:
    // Platform-specific implementation.
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser,
        const CefString& title);

    bool m_is_closing;

    CefRefPtr<CefBrowser> m_current_browser;

    // Handles the browser side of query routing.
    CefRefPtr<CefMessageRouterBrowserSide> m_message_router;
    scoped_ptr<CefMessageRouterBrowserSide::Handler> m_message_handler;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(CefClientHandler);
};