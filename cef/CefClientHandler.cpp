#include "StdAfx.h"

#include "CefClientHandler.h"
#include "MessageHandler.h"

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include <sstream>
#include <string>

namespace {

    CefClientHandler* g_instance = nullptr;

    // Returns a data: URI with the specified contents.
    std::string GetDataURI(const std::string& data, const std::string& mime_type) {
        return "data:" + mime_type + ";base64," +
            CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
            .ToString();
    }

}  // namespace

CefClientHandler::CefClientHandler()
    : m_is_closing(false) {
    DCHECK(!g_instance);
    g_instance = this;
}

CefClientHandler::~CefClientHandler() {
    g_instance = nullptr;
}

// static
CefClientHandler* CefClientHandler::GetInstance() {
    return g_instance;
}

void CefClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
    const CefString& title) {
    CEF_REQUIRE_UI_THREAD();

    // Set the title of the window using platform APIs.
//        PlatformTitleChange(browser, title); // TODO:    
}

void CefClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();

    m_current_browser = browser;

    if (!m_message_router) 
    {
        // Create the browser-side router for query handling.
        CefMessageRouterConfig config;
        m_message_router = CefMessageRouterBrowserSide::Create(config);

        // Register handlers with the router.
        m_message_handler.reset(new MessageHandler());
        m_message_router->AddHandler(m_message_handler.get(), false);
    }
}

bool CefClientHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    
    // Set a flag to indicate that the window close should be allowed.

    m_is_closing = true;
    
    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void CefClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Free the router when the last browser is closed.
    m_message_router->RemoveHandler(m_message_handler.get());
    m_message_handler.reset();
    m_message_router = NULL;
}

void CefClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    ErrorCode errorCode,
    const CefString& errorText,
    const CefString& failedUrl) 
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
        << std::string(failedUrl) << " with error " << std::string(errorText)
        << " (" << errorCode << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void CefClientHandler::CloseBrowser(bool force_close)
{
    if (!CefCurrentlyOn(TID_UI)) {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::Bind(&CefClientHandler::CloseBrowser, this,
            force_close));
        return;
    }

    if (m_current_browser)
        m_current_browser->GetHost()->CloseBrowser(force_close);
}

void CefClientHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback)
{
//TODO:
    //REQUIRE_UI_THREAD();

    //// Continue the download and show the "Save As" dialog.
    //callback->Continue(GetDownloadPath(suggested_name), true);
}

void CefClientHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback)
{
 //TODO:
    //REQUIRE_UI_THREAD();

    //CEFDownloadItemValues values;

    //values.bIsValid = download_item->IsValid();
    //values.bIsInProgress = download_item->IsInProgress();
    //values.bIsComplete = download_item->IsComplete();
    //values.bIsCanceled = download_item->IsCanceled();
    //values.nProgress = download_item->GetPercentComplete();
    //values.nSpeed = download_item->GetCurrentSpeed();
    //values.nReceived = download_item->GetReceivedBytes();
    //values.nTotal = download_item->GetTotalBytes();

    //CString szDispo = download_item->GetContentDisposition().c_str();

    //// The frame window will be the parent of the browser window
    //HWND hWindow = GetParent(browser->GetHost()->GetWindowHandle());

    //// send message
    //::SendMessage(hWindow, WM_APP_CEF_DOWNLOAD_UPDATE, (WPARAM)&values, NULL);
}

bool CefClientHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    CEF_REQUIRE_UI_THREAD();

    return m_message_router->OnProcessMessageReceived(browser, frame, source_process, message);
}

bool CefClientHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{
    CEF_REQUIRE_UI_THREAD();

    m_message_router->OnBeforeBrowse(browser, frame);

    return false;
}

void CefClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status)
{ 
    CEF_REQUIRE_UI_THREAD();

    m_message_router->OnRenderProcessTerminated(browser);
}

void CefClientHandler::SendCalibrationStatus(CalStatuses cal_statuses)
{
    auto& frame = m_current_browser->GetMainFrame();

    auto to_str = [](CompStatus status) {
        switch (status)
        {
        case eUnknown: return "Unknown";
        case eOK: return "OK";
        case eMarginal: return "Marginal";
        case eError: return "Error";
        case eSimulated: return "Simulated";
        case eFatal: return "Fatal";
        case eCalibrating: return "Calibrating";
        default: return "Unknown";
        }
    };

    CStringA result;
    result.Format("onNotify({\"type\": \"notify\", \"cmd\":\"onCalibrationStatus\""
            ", \"CamerasCalStatus\": \"%s\""
            ", \"LightsCalStatus\": \"%s\""
            ", \"CalCupCalStatus\": \"%s\""
            ", \"FPNCalStatus\": \"%s\""
            ", \"WBCalStatus\": \"%s\""
            ", \"OverallCalStatus\": \"%s\""
        "})", 
        to_str(cal_statuses.CamerasCalStatus), 
        to_str(cal_statuses.LightsCalStatus), 
        to_str(cal_statuses.CalCupCalStatus),
        to_str(cal_statuses.FPNCalStatus), 
        to_str(cal_statuses.WBCalStatus), 
        to_str(cal_statuses.OverallCalStatus)
    );

    frame->ExecuteJavaScript(result.GetString(), frame->GetURL(), 0);

}