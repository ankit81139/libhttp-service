#include <stdio.h>
#include "httpserver.hpp"
#include <string>
#include <windows.h>
#include <iostream>
#include <fstream>
#include "service_controller.h"
#include "kuzu.h"
#include <sstream>
#include "rest.hpp"


using namespace std;
using namespace httpserver;
using namespace rest;

SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
SERVICE_STATUS        ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;

void ServiceMain(int argc, char** argv);
void ControlHandler(DWORD request);
void ServiceWorkerThread();
void ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
class hello_world_resource : public http_resource {
    public:
        hello_world_resource(){
            this->API = new restAPI();
        }
        std::shared_ptr<http_response> render(const http_request& req) {
            std::string data = "";
            if(req.get_method() == "GET") {
                data = this->API->get();
            }
            if(req.get_method() == "PUT"){
                this->API->put(req);
            }
            if(req.get_method() == "DELETE"){
                this->API->remove(req);
            }
            if(req.get_method() == "POST"){
                this->API->update(req);
            }
            return std::shared_ptr<http_response>(new string_response("Hello, World! " + data));
        }
    
    private:
        restAPI* API;
};

class handling_multiple_resource : public http_resource {
    public:
        std::shared_ptr<http_response> render(const http_request& req) {
            return std::shared_ptr<http_response>(new string_response("Your URL: " + std::string(req.get_path())));
        }
};

class url_args_resource : public http_resource {
    public:
        std::shared_ptr<http_response> render(const http_request& req) {
            std::string arg1(req.get_arg("arg1"));
            std::string arg2(req.get_arg("arg2"));
            return std::shared_ptr<http_response>(new string_response("ARGS: " + arg1 + " and " + arg2));
        }
};

int main(int argc, char* argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[2];
    ServiceTable[0].lpServiceName = const_cast<LPSTR>("KuzuService");
    ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
    ServiceTable[1].lpServiceName = NULL;
    ServiceTable[1].lpServiceProc = NULL;
    StartServiceCtrlDispatcher(ServiceTable);

    if (argc <= 1) {
        return 0;
    }
    string command = argv[1];
    std::wstring serviceName = L"KuzuService";
    controller::winService ControlHandler = controller::winService(serviceName, serviceName);
    if (command == "start") {
        ControlHandler.start();
    }
    else if (command == "install")
    {
        ControlHandler.install(argv);
    }
    else if (command == "uninstall")
    {
        ControlHandler.uninstall();
    }
    else if(command == "stop"){
        ControlHandler.stop();
    }
}

void ServiceMain(int argc, char** argv)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    hStatus = RegisterServiceCtrlHandlerA("KuzuService", (LPHANDLER_FUNCTION)ControlHandler);

    if (hStatus == (SERVICE_STATUS_HANDLE)0)
    {
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        ServiceWorkerThread();   
    }
}

void ControlHandler(DWORD request)
{
    if (request == SERVICE_CONTROL_STOP || request == SERVICE_CONTROL_SHUTDOWN)
    {
        ServiceStatus.dwWin32ExitCode = 0;
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &ServiceStatus);
        return;
    }
}

void ServiceWorkerThread()
{
    webserver ws = create_webserver(8080);
    hello_world_resource hwr;
    ws.register_resource("/hello", &hwr);

    handling_multiple_resource hmr;
    ws.register_resource("/family", &hmr, true);
    ws.register_resource("/with_regex_[0-9]+", &hmr);

    url_args_resource uar;
    ws.register_resource("/url/with/{arg1}/and/{arg2}", &uar);
    ws.register_resource("/url/with/parametric/args/{arg1|[0-9]+}/and/{arg2|[A-Z]+}", &uar);

    ws.start(true);
    ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    // Update the service status
    g_ServiceStatus.dwCurrentState       = dwCurrentState;
    g_ServiceStatus.dwWin32ExitCode      = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint           = dwWaitHint;

    // Report the status to the SCM.
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}