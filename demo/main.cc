#include <iostream>
#include <vector>
#include "api_poe_dhcp_server.h"
#include "log.h"
#include <stdio.h>
#include <Windows.h>
#include "Poco/Thread.h"
const std::string tag = "main";
#define SERVICENAME  "dhcp-server"
#define SLEEP_TIME 2000 //间隔时间
bool brun = false;

SERVICE_STATUS servicestatus;
SERVICE_STATUS_HANDLE hstatus;

std::shared_ptr<dhcpserver::PoeDhcpSever> poe_dhcp_server = nullptr;

void WINAPI ServiceMain(int argc, char** argv);
void WINAPI CtrlHandler(DWORD request);
int InitService();
bool StartDhcpServer();
bool StopDhcpSrever();

void WINAPI ServiceMain(int argc, char** argv)
{
    servicestatus.dwServiceType = SERVICE_WIN32;
    servicestatus.dwCurrentState = SERVICE_START_PENDING;
    servicestatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;//在本例中只接受系统关机和停止服务两种控制命令
    servicestatus.dwWin32ExitCode = 0;
    servicestatus.dwServiceSpecificExitCode = 0;
    servicestatus.dwCheckPoint = 0;
    servicestatus.dwWaitHint = 0;
    hstatus = ::RegisterServiceCtrlHandler(SERVICENAME, CtrlHandler);
    if (hstatus == nullptr){
        LOG_INFO(tag, "RegisterServiceCtrlHandler failed");
        return;
    }
    LOG_INFO(tag, "RegisterServiceCtrlHandler success");
    //向SCM 报告运行状态
    servicestatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hstatus, &servicestatus);
    //在此处添加你自己希望服务做的工作，在这里我做的工作是获得当前可用的物理和虚拟内存信息
    brun = true;
    MEMORYSTATUS memstatus;
    StartDhcpServer();
    while (brun)
    {
        LOG_INFO(tag, "service is running ");
        Poco::Thread::sleep(SLEEP_TIME);
    }
    StopDhcpSrever();
    LOG_INFO(tag, "service stopped");
}

void WINAPI CtrlHandler(DWORD request)
{
    switch (request)
    {
    case SERVICE_CONTROL_STOP:
        brun = false;
        servicestatus.dwCurrentState = SERVICE_STOPPED;
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        brun = false;
        servicestatus.dwCurrentState = SERVICE_STOPPED;
        break;

    default:
        break;
    }
    SetServiceStatus(hstatus, &servicestatus);
}

int InitService()
{
    SERVICE_TABLE_ENTRY entrytable[2];
    entrytable[0].lpServiceName = SERVICENAME;
    entrytable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
    entrytable[1].lpServiceName = nullptr;
    entrytable[1].lpServiceProc = nullptr;
    StartServiceCtrlDispatcher(entrytable);
    return 0;
}

bool StartDhcpServer()
{
    if(!poe_dhcp_server)
        poe_dhcp_server = dhcpserver::PoeDhcpSever::create();
    dhcpserver::LOG::SetPath("C:\\Users\\MAXHUB\\Desktop\\testing\\dhcp.log");
    // poe_dhcp_server->SetPoeAdapterModel("Intel(R) Ethernet");
    bool ret = false;
    poe_dhcp_server->StopDhcpServers();
    poe_dhcp_server->ConfigPoeAdapter();
    Poco::Thread::sleep(SLEEP_TIME);
    do {
        ret = poe_dhcp_server->StartDhcpServers();
        Poco::Thread::sleep(SLEEP_TIME);
    } while (!ret);
    
    return true;
}

bool StopDhcpSrever()
{
    if(poe_dhcp_server) {
        poe_dhcp_server->StopDhcpServers();
    }
    return true;
}

int main()
{
    // StartDhcpServer();
    // while (true)
    // {
    //     
    // }
    // StopDhcpSrever();
    // return 0;
    return InitService();
}
