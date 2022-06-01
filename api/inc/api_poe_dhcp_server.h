#ifndef POE_DHCP_SERVER_H
#define POE_DHCP_SERVER_H
#include <string>
#include <sstream>

namespace dhcpserver
{

	struct AdapterAddr
	{
		std::string ip;
		std::string mask;
		std::string gayte_way;
		std::string dns;
		std::string dns2;
		std::string min_ip;
		std::string max_ip;
	};

    class PoeDhcpSever
    {
    public:

        static std::shared_ptr<PoeDhcpSever> create();

		static void LogInfo(std::string tag, std::stringstream &ss);

		static void SetLogPath(std::string path);

        virtual ~PoeDhcpSever() = default;

        virtual void ConfigPoeAdapter() = 0;

		virtual int GetPoeAdapterCount() = 0;

		virtual bool SetPoeAdapterAddress(const int index, const AdapterAddr &address) = 0;

		virtual bool StartDhcpServers() = 0;

		virtual bool StopDhcpServers() = 0;

		virtual void SetPoeAdapterModel(std::string model) = 0;
    };
}
#endif