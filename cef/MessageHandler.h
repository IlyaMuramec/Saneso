#pragma once

#include "include/wrapper/cef_message_router.h"

// Handle messages in the browser process.
class MessageHandler : public CefMessageRouterBrowserSide::Handler 
{
public:

    MessageHandler() {}
    
    // Called due to cefQuery execution in message_router.html.
    virtual bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64 query_id, const CefString& request, bool persistent, CefRefPtr<Callback> callback) OVERRIDE;

private:
    
    DISALLOW_COPY_AND_ASSIGN(MessageHandler);
};