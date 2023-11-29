#include "service_controller.h"
#include <iostream>

using namespace controller;
winService::winService(std::wstring serviceName, std::wstring ServiceDisplayName)
{
    this->serviceName = ServiceDisplayName;
    this->serviceName = serviceName;
}

void winService::start()
{
    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm == NULL)
    {
        std::cerr << "Failed to open service control manager\n";
        return;
    }

    service = OpenServiceW(scm, serviceName.c_str(), SERVICE_START);
    if (service == NULL)
    {
        DWORD error = GetLastError();
        std::cerr << "Failed to open service. Error code: " << error << "\n";
        return;
    }
    bool res = StartServiceW(service, 0, NULL);
    if (!res)
    {
        DWORD error = GetLastError();
        std::cerr << "Failed to start service with Error code : " << error << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return;
    }

    std::cout << "Service starting...\n";

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}

void winService::install(char *argv[])
{
    // Install the service using sc create
    int wideArgv0Size = MultiByteToWideChar(CP_UTF8, 0, argv[0], -1, NULL, 0);
    std::wstring wideArgv0(wideArgv0Size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, argv[0], -1, &wideArgv0[0], wideArgv0Size);

    std::wstring serviceBinaryPath = L"" + wideArgv0;

    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
    {
        std::cerr << "Failed to open Service Control Manager. Error code: " << GetLastError() << "\n";
        return;
    }

    service = CreateServiceW(
        scm,
        serviceName.c_str(),
        serviceDisplayName.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        serviceBinaryPath.c_str(),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);

    if (!service)
    {
        std::cerr << "Failed to create service. Error code: " << GetLastError() << "\n";
        CloseServiceHandle(scm);
        return;
    }

    std::wcout << L"Service installed successfully.\n";
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}

void winService::stop()
{
    scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm)
    {
        std::cerr << "Failed to open Service Control Manager. Error code: " << GetLastError() << "\n";
        return;
    }
    // LPCSTR serviceNarrowName = serviceName.c_str();
    service = OpenServiceW(scm, serviceName.c_str(), SERVICE_STOP);
    if (!service)
    {
        std::cerr << "Failed to open service for stopping. Error code: " << GetLastError() << "\n";
    }
    else
    {
        SERVICE_STATUS serviceStatus;
        if (ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus))
        {
            std::wcout << L"Service stop pending...\n";
        }
        else
        {
            std::cerr << "Failed to request service stop. Error code: " << GetLastError() << "\n";
        }
        CloseServiceHandle(service);
    }
}

void winService::uninstall()
{
    // Uninstall the service using sc delete
    std::string uninstallCommand = "sc delete KuzuService";
    int result = system(uninstallCommand.c_str());

    if (result == 0)
    {
        std::cout << "Service uninstalled successfully.\n";
    }
    else
    {
        std::cerr << "Failed to uninstall service.\n";
    }
}