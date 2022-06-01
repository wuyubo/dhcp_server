#ifndef POE_DHCP_SERVER_IMPL_H
#define POE_DHCP_SERVER_IMPL_H
#include "api_poe_dhcp_server.h"
#include "dhcp_server.h"
#include "network_adapter.h"
#include <thread>
#include "Poco/Thread.h"

namespace dhcpserver
{
	typedef  std::vector<std::shared_ptr<DhcpServer>>  DhcpServerList;

    class PoeDhcpSeverImpl : public PoeDhcpSever 
    {
    public:
		PoeDhcpSeverImpl();

        ~PoeDhcpSeverImpl();

        void ConfigPoeAdapter() override;

		int GetPoeAdapterCount() override;

		bool SetPoeAdapterAddress(const int index, const AdapterAddr &address) override;

		bool StartDhcpServers() override;

		void SetPoeAdapterModel(const std::string model) override;

		bool StopDhcpServers() override;

	private:
        bool IsMainAdatpter(const NetWorkAapterInfo& info) const;

		bool IsPoeAdapter(const NetWorkAapterInfo& info) const;

        NetWorkAapterInfo GetMainAdapterInfo();

        NetWorkIp CreatePoeAdapterAddress(const std::vector<NetWorkIp>& confict_ip) const;

        std::vector<DhcpNetWorkAddr> GetDhcpAddressList(const std::vector<NetWorkIp>& ips) const;

    private:
		std::unique_ptr<NetWorkAdapter> network_adapter_;
		DhcpServerList                  dhcp_server_list_;
		std::string                     poe_adapter_model_;
    };
}
#endif