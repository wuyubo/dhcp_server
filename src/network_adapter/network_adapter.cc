#include "network_adapter.h"
#include "log.h"
#include "net_config.h"
#include <Iphlpapi.h>
#pragma comment(lib,"Iphlpapi.lib")


#pragma execution_character_set("utf-8")

namespace dhcpserver
{
	static const std::string tag = "adapter";
    NetWorkAdapter::NetWorkAdapter()
    {
        GetAdapters();
    }

    NetWorkAdapter::~NetWorkAdapter()
    {
		GetAdapters();
    }

 
    void NetWorkAdapter::GetAdapters()
    {
        auto p_ip_adapter_info = new IP_ADAPTER_INFO();
        unsigned long st_size = sizeof(IP_ADAPTER_INFO);
        int n_rel = GetAdaptersInfo(p_ip_adapter_info, &st_size);
        if (ERROR_BUFFER_OVERFLOW == n_rel)
        {
            delete p_ip_adapter_info;
            p_ip_adapter_info = reinterpret_cast<PIP_ADAPTER_INFO>(new BYTE[st_size]);
            n_rel = GetAdaptersInfo(p_ip_adapter_info, &st_size);
        }
        if (ERROR_SUCCESS == n_rel)
        {
            adapter_infos_.clear();
            while (p_ip_adapter_info)
            {
                NetWorkAapterInfo info;
 
                info.adapter_name = p_ip_adapter_info->AdapterName;
                info.description = p_ip_adapter_info->Description;

                IP_ADDR_STRING *pIpAddrString = &(p_ip_adapter_info->IpAddressList);
                info.ip.address = pIpAddrString->IpAddress.String;
                info.ip.mask = pIpAddrString->IpMask.String;
                info.ip.gate_way = p_ip_adapter_info->GatewayList.IpAddress.String;
                p_ip_adapter_info = p_ip_adapter_info->Next;

                adapter_infos_.push_back(info);
            }
        }
        delete p_ip_adapter_info;
    }

    std::vector<NetWorkAapterInfo> NetWorkAdapter::get_adapter_infos()
    {
        return adapter_infos_;
    }

	void NetWorkAdapter::ConfigIp(std::string adapter_name, NetWorkIp ip)
	{
		NetConfig config;
		config.set_key(adapter_name);
		LOG_INFO(tag, "Config "<< adapter_name << " ip to" << ip.address);
		// if (config.enable_dhcp()) {
		// 	LOG_INFO(tag, "enable_dhcp success");
		// }
		// else {
		// 	LOG_INFO(tag, "enable_dhcp error code: " << config.get_last_error_code());
		// }

		if (config.set_ip_config(ip.address, ip.mask, ip.gate_way)) {
			LOG_INFO(tag, "set_ip_config success: " << ip.address);
		}
		else {
			LOG_INFO(tag, "set_ip_config error code: " << config.get_last_error_code());
		}

		if (config.set_dns(ip.dns, ip.dns2)) {
			LOG_INFO(tag, "set_dns success");
		}
		else {
			LOG_INFO(tag, "set_dns error code: " << config.get_last_error_code());
		}

	}

    void NetWorkAdapter::print()
    {
		LOG_INFO(tag, "adapter infos:");
        for (auto it : adapter_infos_)
        {
            LOG_INFO(tag, "=============================");
			printInfo(it);
        }
    }

	void NetWorkAdapter::printInfo(NetWorkAapterInfo info)
	{
		LOG_INFO(tag, "name: " << info.adapter_name);
		LOG_INFO(tag, "description: " << info.description);
		LOG_INFO(tag, "ip: " << info.ip.address);
		LOG_INFO(tag, "mask: " << info.ip.mask);
		LOG_INFO(tag, "gateway: " << info.ip.gate_way);
	}



}
