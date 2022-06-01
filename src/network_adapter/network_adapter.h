/*
* Author: Manash Kumar Mandal
* Modified Library introduced in Arduino Playground which does not work
* This works perfectly
* LICENSE: MIT
*/

#pragma once

#define ARDUINO_WAIT_TIME 2000
#define MAX_DATA_LENGTH 255

#include <windows.h>
#include <string>
#include <vector>


namespace dhcpserver
{
    struct NetWorkIp
    {
        std::string address;
        std::string mask;
        std::string gate_way;
		std::string dns;
		std::string dns2;
        NetWorkIp():
            address("0.0.0.0"),
            mask("255.255.255.0"),
            gate_way("0.0.0.0"),
            dns("0.0.0.0"),
            dns2("0.0.0.0"){}
    };

    struct NetWorkAapterInfo
    {
        std::string adapter_name;
        std::string description;
        NetWorkIp ip;
        NetWorkAapterInfo():
           adapter_name(""), 
           description(""){}
    };

    class NetWorkAdapter
    {
    public:
        explicit NetWorkAdapter();

        ~NetWorkAdapter();

        std::vector<NetWorkAapterInfo> get_adapter_infos();

		void ConfigIp(std::string adapter_name, NetWorkIp ip);

        void print();

		void printInfo(NetWorkAapterInfo info);
    private:
        void GetAdapters();

    private:
        std::vector<NetWorkAapterInfo> adapter_infos_;
    };
}
