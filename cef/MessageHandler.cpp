#include "StdAfx.h"

#include "MessageHandler.h"

#include "../NGEuserApp.h"
#include "../NGEuserDlg.h"
#include "../CameraDlg.h"

// disable warning 4003 because compiler is confusing
#undef max
#undef min

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <sstream>

bool MessageHandler::OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64 query_id, const CefString& request, bool persistent, CefRefPtr<Callback> callback)
{    
    // Only handle messages from the startup URL.
//    const std::string& url = frame->GetURL();
//    if (url.find(startup_url_) != 0)
//        return false;

    const std::string message(request);

    using namespace rapidjson;

    Document doc;
    ParseResult parse_result = doc.Parse(message);

    if (!parse_result)
    {
        CStringA result;
        result.Format("JSON parse error: %s (%u)", GetParseError_En(parse_result.Code()), parse_result.Offset());

        callback->Success(result.GetString());
        return true;
    }

    CNGEuserApp* theApp = static_cast<CNGEuserApp*>(AfxGetApp());

    if (doc.HasMember("cmd"))
    {
        const std::string& cmd = doc["cmd"].GetString();

        if (cmd == "guiapi_setViewMode")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_startCalibration")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_videoMode")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_videoPlay")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_videoList")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_setLights")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_start")
        {
            AfxMessageBox(std::wstring(request).data());
        }
        else if (cmd == "guiapi_nextImageStitchingMode")
        {
            theApp->m_SystemManager.NextImageStitchingMode();
        }
        else if (cmd == "guiapi_selectBandImaging")
        {
            theApp->m_pMainDlg->m_dMainDlg.OnBnClickedButtonSelectBandImaging();
        }
        else if (cmd == "guiapi_ledFrontAuto")
        {
            theApp->m_pMainDlg->m_dLightingDlg.OnBnClickedLedFrontAuto();
        }
        else if (cmd == "guiapi_ledFrontUp")
        {
            theApp->m_pMainDlg->m_dLightingDlg.OnBnClickedLedFrontUp();
        }
        else if (cmd == "guiapi_ledFrontDown")
        {
            theApp->m_pMainDlg->m_dLightingDlg.OnBnClickedLedFrontDown();
        }
        else if (cmd == "guiapi_setLedFrontValue")
        {
            if (doc.HasMember("pos"))
            {
                //void CLightControl::SetFrontLightsLevel(float fLevel, bool bUpdateMC)

                const auto pos = doc["pos"].GetInt();
                theApp->m_pMainDlg->m_dLightingDlg.m_sldrFrontValue.SetPos(pos);
            }            
        }
        else if (cmd == "guiapi_recordVideoStart")
        {
            theApp->GetSystemManager()->CaptureVideoFile();
        }
        else if (cmd == "guiapi_recordVideoStop")
        {
            theApp->GetSystemManager()->StopCapture();
        }
        else if (cmd == "guiapi_doCapture")
        {
            theApp->m_pMainDlg->m_dMainDlg.OnBnClickedButtonDoCapture();
        }
        else if (cmd == "guiapi_doFreezeAction")
        {
            DisplayMode mode = theApp->GetSystemManager()->GetDisplayMode();
            if (mode == eLive)
            {
                theApp->GetSystemManager()->SetNewDisplayMode(eFrozen); 
            }
        }
        else if (cmd == "guiapi_doLiveAction")
        {
            DisplayMode mode = theApp->GetSystemManager()->GetDisplayMode();
            if (mode != eLive)
            {
                theApp->GetSystemManager()->SetNewDisplayMode(eLive);
            }
        }
        else if (cmd == "guiapi_doMarkersOnOff")
        {
            theApp->m_pMainDlg->m_dHandsetDlg.DoAction(MARKERS_ON_OFF);
        }
        else if (cmd == "guiapi_startSelfCal")
        {
            auto check_and_set = [&doc, &theApp](const char* name, int nID) 
            {
                if (doc.HasMember(name))
                {
                    ((CButton*)theApp->m_pMainDlg->m_dSelfCalibrationDlg.GetDlgItem(nID))->SetCheck(doc[name].GetBool());
                }
            };

            check_and_set("ledCameras", IDC_LED_CAL_CAMERAS_OK);
            check_and_set("ledCalCup", IDC_LED_CAL_CUP_CHECK);
            check_and_set("ledLights", IDC_LED_CAL_LIGHTS_OK);
            check_and_set("ledFPN", IDC_LED_CAL_FPN);
            check_and_set("ledWB", IDC_LED_CAL_WB);
            check_and_set("ledOverall", IDC_LED_CAL_OVERALL);            

            theApp->m_pMainDlg->m_dSelfCalibrationDlg.OnBnClickedButtonStartSelfCal();
        }
        else if (cmd == "guiapi_exit")
        {
            theApp->CloseCefBrowser();
        }
        else
        {
            AfxMessageBox(_T("Unknown API"));
        }

        // send confirmation        
        {        
            CStringA confirm;
            confirm.Format("onNotify({\"type\": \"confirmation\", \"cmd\":\"%s\"})", cmd.data());

            frame->ExecuteJavaScript(confirm.GetString(), frame->GetURL(), 0);
        }               

        // send Gain
        {
            const int gain = theApp->GetSystemManager()->GetCameraData(eFront)->GetGain();
            CStringA result;
            result.Format("onNotify({\"type\": \"notify\", \"cmd\":\"onChangeGain\", \"value\": %d})", gain);

            frame->ExecuteJavaScript(result.GetString(), frame->GetURL(), 0);
        }


        return true;

        // Execiting API callbacks

        CefRefPtr<CefFrame> frame = browser->GetMainFrame();
        frame->ExecuteJavaScript(R"(onNotify({"notify":"onStatusChange", "type": 0, "status": 0}))", frame->GetURL(), 0);
        frame->ExecuteJavaScript(R"(onNotify({"notify":"onGeneralMessage", "type": 0, "message": "some text"}))", frame->GetURL(), 0);

    }

    return true;
    

           
        // Reverse the string and return.
        //std::string result = message_name.substr(sizeof(kTestMessageName));
        //std::reverse(result.begin(), result.end());
        //callback->Success(result);
        //return true;
 
       
    return false;
}