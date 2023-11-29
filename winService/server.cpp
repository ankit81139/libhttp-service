#include <stdio.h>
#include "httpserver.hpp"
#include <string>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <C:\src\http-service\winService\service_controller.h>
#include <C:\src\http-service\winService\kuzu.h>


using namespace std;
//.
namespace
{
    // Global variable for service status
    SERVICE_STATUS        g_ServiceStatus = {0};
    SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
}

class hello_world_resource : public httpserver::http_resource {
 public:
     std::shared_ptr<httpserver::http_response> render(const httpserver::http_request&);
     void set_some_data(const std::string &s) {data = s;}
     std::string data;
};

// Using the render method you are able to catch each type of request you receive
std::shared_ptr<httpserver::http_response> hello_world_resource::render(const httpserver::http_request& req) {
    // It is possible to store data inside the resource object that can be altered through the requests
    std::cout << "Data was: " << data << std::endl;
    std::string_view datapar = req.get_arg("data");
    set_some_data(datapar == "" ? "no data passed!!!" : std::string(datapar));
    std::cout << "Now data is:" << data << std::endl;

    // It is possible to send a response initializing an http_string_response that reads the content to send in response from a string.
    return std::shared_ptr<httpserver::http_response>(new httpserver::string_response("Hello World!!!", 200));
}

SERVICE_STATUS        ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;

void ServiceMain(int argc, char** argv);
void ControlHandler(DWORD request);
void ServiceWorkerThread();
void ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void kuzu();


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
    kuzu();

    httpserver::webserver ws = httpserver::create_webserver(8080).start_method(httpserver::http::http_utils::INTERNAL_SELECT).max_threads(5);

    hello_world_resource hwr;
    // This way we are registering the hello_world_resource to answer for the endpoint
    // "/hello". The requested method is called (if the request is a GET we call the render_GET
    // method. In case that the specific render method is not implemented, the generic "render"
    // method is called.
    ws.register_resource("/hello", &hwr, true);

    // This way we are putting the created webserver in listen. We pass true in order to have
    // a blocking call; if we want the call to be non-blocking we can just pass false to the method.
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

void kuzu()
{
    ofstream logFile("C:\\log2.txt", ios::app);
    logFile << " service started" << endl;
    kuzu_database* db = kuzu_database_init("C:\\kuzu-test", kuzu_default_system_config());
    kuzu_connection* conn = kuzu_connection_init(db);
    kuzu_query_result* result = kuzu_connection_query(conn, "CREATE NODE TABLE Person(name STRING, age INT64, PRIMARY KEY(name));");
    result = kuzu_connection_query(conn, "CREATE (:Person {name: 'Alice', age: 25});");
    result = kuzu_connection_query(conn, "CREATE (:Person {name: 'Bob', age: 30});");
    result = kuzu_connection_query(conn, "MATCH (a:Person) RETURN a.name AS NAME, a.age AS AGE;");
    while (kuzu_query_result_has_next(result)) {
        kuzu_flat_tuple* tuple = kuzu_query_result_get_next(result);
        kuzu_value* value = kuzu_flat_tuple_get_value(tuple, 0);
        char* name = kuzu_value_get_string(value);
        value = kuzu_flat_tuple_get_value(tuple, 1);
        int64_t age = kuzu_value_get_int64(value);
        kuzu_value_destroy(value);
        logFile << age << " " << name << endl;
        kuzu_flat_tuple_destroy(tuple);
    }
    kuzu_query_result_destroy(result);
    kuzu_connection_destroy(conn);
    kuzu_database_destroy(db);
}