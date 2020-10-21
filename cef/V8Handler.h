#pragma once

#include "include/cef_v8.h"

class V8Handler : public CefV8Handler {
public:
    V8Handler() {}

    virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) OVERRIDE;

private:

    // Provide the reference counting implementation for this class.
    IMPLEMENT_REFCOUNTING(V8Handler);
};