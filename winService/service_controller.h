#ifndef INCLUDED_SERVICE_CONTROLLER
#define INCLUDED_SERVICE_CONTROLLER

#include <windows.h>
#include <string>

//namespace controller
namespace controller
{
    class winService
    {
    public:
        winService( std::wstring serviceName, std::wstring serviceDisplayName);
        
        void install(char* argv[]);
        void uninstall();
        void stop();
        void start();

    private:
        std::wstring serviceName, serviceDisplayName;
        SC_HANDLE scm, service;
    };

} // namespace http

#endif