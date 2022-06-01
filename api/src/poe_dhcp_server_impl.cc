#include "../../version.h"
#include "poe_dhcp_server_impl.h"
#include "log.h"
#include <vector>
#include <atomic>

namespace dhcpserver
{
	static const std::string log_tag = "PoeDhcpApi";
	static  std::atomic_bool stop_dhcp_server_ = false;
    const std::string kStartPoeIpHead = "192.168.";
    const uint32_t kStartPoeIpSegment = 50;

	std::shared_ptr<PoeDhcpSever> PoeDhcpSever::create()
	{
		std::shared_ptr<PoeDhcpSever> server(new PoeDhcpSeverImpl());
		return server;
	}

	void PoeDhcpSever::LogInfo(std::string tag, std::stringstream &ss)
	{
		LOG_INFO(tag, ss.str());
	}

	void PoeDhcpSever::SetLogPath(std::string path)
	{
		LOG::SetPath(path);
	}

	PoeDhcpSeverImpl::PoeDhcpSeverImpl(): 
		poe_adapter_model_("Intel(R) I211")
	{
		network_adapter_.reset(new NetWorkAdapter());
		//network_adapter_->print();
	}

	PoeDhcpSeverImpl::~PoeDhcpSeverImpl()
	{
        StopDhcpServers();
	}

    void PoeDhcpSeverImpl::ConfigPoeAdapter() {
        
        auto infos = network_adapter_->get_adapter_infos();
        network_adapter_->print();
        auto main_info = GetMainAdapterInfo();
        std::vector<NetWorkIp> config_ip_list;
        config_ip_list.push_back(main_info.ip);
        LOG_INFO(log_tag, "config poe adapter");
        for (auto it : infos){
            if (IsPoeAdapter(it)){
                auto ip = CreatePoeAdapterAddress(config_ip_list);
                config_ip_list.push_back(ip);
                network_adapter_->ConfigIp(it.adapter_name, ip);
            }
        }
        for(auto it = config_ip_list.begin();  it != config_ip_list.end(); ++it) {
            if(it->address == main_info.ip.address) {
                config_ip_list.erase(it);
                break;
            }
        }
        dhcp_server_list_.clear();
        auto address_list = GetDhcpAddressList(config_ip_list);
        LOG_INFO(log_tag, "create dhcp servers count: " << address_list.size());
        for(auto it: address_list) {
            std::unique_ptr<DhcpServer> dhcp_server(new DhcpServer(it));
            dhcp_server_list_.push_back(std::move(dhcp_server));
        }
    }

    int PoeDhcpSeverImpl::GetPoeAdapterCount()
	{
		int poe_count = 0;
		std::vector<NetWorkAapterInfo> infos = network_adapter_->get_adapter_infos();
		for (auto it : infos)
		{
			if (IsPoeAdapter(it))
			{
				poe_count++;
			}
		}
		return poe_count;
	}

	bool PoeDhcpSeverImpl::SetPoeAdapterAddress(const int index, const AdapterAddr &address)
	{
		LOG_INFO(log_tag, "Set Poe "<< index <<" adapter Ip: " << address.ip);
		std::vector<NetWorkAapterInfo> infos = network_adapter_->get_adapter_infos();
		int poe_index = 0;
		int hard_adapter_index = 0;
		NetWorkAapterInfo info;
		for (auto it : infos)
		{
			if (IsPoeAdapter(it) && poe_index++ == index)
			{
				info = it;
				break;
			}
		}

		if (info.ip.address == address.ip)
		{
			LOG_INFO(log_tag, "The IP address is already correct and does not need to be set.");
			return true;
		}
		if (info.ip.address != "")
		{
			NetWorkIp ip;
			ip.address = address.ip;
			ip.mask = address.mask;
			ip.gate_way = address.gayte_way;
			ip.dns = address.dns;
			ip.dns2 = address.dns2;

			network_adapter_->ConfigIp(info.adapter_name, ip);
			return true;
		}

		return false;
	}

	bool PoeDhcpSeverImpl::StartDhcpServers()
	{
        bool ret = false;
        LOG_INFO(log_tag, "start dhcp servers... ");
        for(auto it: dhcp_server_list_) {
            it->Start();
            ret = it->IsRunning();
        }
        return ret;
	}

	bool PoeDhcpSeverImpl::StopDhcpServers()
	{
        LOG_INFO(log_tag, "stop dhcp servers... ");
		for(auto it: dhcp_server_list_) {
            it->Stop();
		}
		return true;
	}

    bool PoeDhcpSeverImpl::IsMainAdatpter(const NetWorkAapterInfo& info) const {
        const auto flag = info.description.find("Intel(R) Ethernet");
        return flag != std::string::npos;
    }

    bool PoeDhcpSeverImpl::IsPoeAdapter(const NetWorkAapterInfo& info) const {
        const auto flag = info.description.find(poe_adapter_model_);
        return flag != std::string::npos;
	}

    NetWorkAapterInfo PoeDhcpSeverImpl::GetMainAdapterInfo() {
        auto infos = network_adapter_->get_adapter_infos();
        for (auto it : infos) {
            if (IsMainAdatpter(it)) {
                return it;
            }
        }
        return NetWorkAapterInfo();
    }

    NetWorkIp PoeDhcpSeverImpl::CreatePoeAdapterAddress(const std::vector<NetWorkIp>& confict_ip) const {
        NetWorkIp net_work_ip;
        std::string target_gateway = kStartPoeIpHead + std::to_string(kStartPoeIpSegment) + ".1";
        uint32_t poe_ip_segment = kStartPoeIpSegment;
        for(auto it: confict_ip) {
            if(it.gate_way == target_gateway) {
                poe_ip_segment++;
                target_gateway = kStartPoeIpHead + std::to_string(poe_ip_segment) + ".1";
            }
        }
        net_work_ip.address = target_gateway;
        net_work_ip.gate_way = target_gateway;
        LOG_INFO(log_tag, "create poe ip: " << net_work_ip.address);
        return net_work_ip;
    }

    std::vector<DhcpNetWorkAddr> PoeDhcpSeverImpl::GetDhcpAddressList(const std::vector<NetWorkIp>& ips) const {
        std::vector<DhcpNetWorkAddr> address_list;
        const uint32_t max_addr = 254;
        const uint32_t step = 10;
        uint32_t min_addr = 2;
        for(auto it: ips) {
            DhcpNetWorkAddr addr;
            addr.address = it.address;
            addr.mask = it.mask;
            addr.dns = it.dns;
            addr.dns2 = it.dns2;
            std::string segment = it.gate_way.substr(0, it.gate_way.length()-1);
            addr.min_addr = segment + std::to_string(min_addr);
            min_addr = min_addr + step;
            if(min_addr > max_addr)
                addr.max_addr = segment + std::to_string(max_addr);
            else
                addr.max_addr = segment + std::to_string(min_addr);
            min_addr++;
            address_list.push_back(addr);
        }
        return address_list;
    }


    void PoeDhcpSeverImpl::SetPoeAdapterModel(std::string model)
	{
		poe_adapter_model_ = model;
	}
}
