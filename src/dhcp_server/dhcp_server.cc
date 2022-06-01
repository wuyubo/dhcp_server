#include "dhcp_server.h"
#include "log.h"
#include <vector>

namespace dhcpserver
{
#define DWIP0(dw) (((dw)>> 0) & 0xff)
#define DWIP1(dw) (((dw)>> 8) & 0xff)
#define DWIP2(dw) (((dw)>>16) & 0xff)
#define DWIP3(dw) (((dw)>>24) & 0xff)
#define DWIPtoValue(dw) ((DWIP0(dw)<<24) | (DWIP1(dw)<<16) | (DWIP2(dw)<<8) | DWIP3(dw))
#define DWValuetoIP(dw) ((DWIP0(dw)<<24) | (DWIP1(dw)<<16) | (DWIP2(dw)<<8) | DWIP3(dw))

#define DHCP_SERVER_PORT (67)
#define DHCP_CLIENT_PORT (68)

    // Broadcast bit for flags field (RFC 2131 section 2)
#define BROADCAST_FLAG (0x80)

#define MAX_HOSTNAME_LENGTH 256

    // Maximum size of a UDP datagram (see RFC 768)
#define MAX_UDP_MESSAGE_SIZE ((65536)-8)

    struct ClientIdentifierData
    {
        const BYTE* pbClientIdentifier;
        DWORD dwClientIdentifierSize;
    };

    // DHCP magic cookie values
    const BYTE pbDHCPMagicCookie[] = { 99, 130, 83, 99 };


	DhcpServer::DhcpServer(DhcpNetWorkAddr net_address) :
                        net_address_(net_address), 
                        starting_(false), 
                        thread_(nullptr)
    {
		tag_ = "DhcpServer." + net_address_.address;
		LOG_INFO(tag_, "Dhcp Server ip: " << net_address_.address);
		LOG_INFO(tag_, "Ip allocation range (" << net_address_.min_addr << "-" << net_address_.max_addr << ")");
        SetServerAddress(net_address);
		last_offer_address_ = DWIPtoValue(server_address_.max_address);
    }

    DhcpServer::~DhcpServer()
    {
        for (auto it : addresses_in_use_)
        {
            if (it.pbClientIdentifier)
            {
                LocalFree(it.pbClientIdentifier);
            }
        }
    }

    bool DhcpServer::Start() {
        if(starting_){
            Stop();
        }
        if (InitServer()) {
			LOG_INFO(tag_, "Dhcp Server start ... ");
			starting_ = true;
            thread_.reset(new Poco::Thread("dhcp"));
            thread_->start(*this);
            return true;
        }
        return false;
    }

    bool DhcpServer::Stop() {
        if (thread_) {
            starting_ = false;
            thread_->join();
        }
        return true;
    }

    bool DhcpServer::IsRunning() {
        return starting_;
    }

    void DhcpServer::run() {
        int bytes_received = 0;
        char read_buffer[MAX_UDP_MESSAGE_SIZE];
        if (!dhcp_socket_) {
            LOG_INFO(tag_, "not init ... ");
            Stop();
            return;
        }

        while(starting_) {
            LOG_INFO(tag_, "Waiting dhcp message ... ");
            bytes_received = dhcp_socket_->receiveBytes(read_buffer, sizeof(read_buffer) - 1);
            if(bytes_received < 0)  {
                LOG_INFO(tag_, "receive error: " << bytes_received);
                continue;
            }

            const DHCPMessage* const request = ParseReceiveData(read_buffer, bytes_received);
            if(!request) {
                LOG_INFO(tag_, "ParseReceiveData error! ");
                continue;
            }

            uint32_t options_size = 0;
            const BYTE* const options = GetOptions(request, bytes_received, &options_size);
            if (!options) {
                LOG_INFO(tag_, "GetOptions error! ");
                continue;
            }

            DHCPMessageTypes message_type = DHCPMessageType_NONE;
            if (!GetDHCPMessageType(options, options_size, &message_type)) {
                LOG_INFO(tag_, "GetDHCPMessageType error! ");
                continue;
            }

            bool ingnore = false;
            switch (message_type) {
            case DHCPMessageType_OFFER:
            case DHCPMessageType_DECLINE:
            case DHCPMessageType_ACK:
            case DHCPMessageType_NAK:
            case DHCPMessageType_RELEASE:
            case DHCPMessageType_INFORM:
                ingnore = true;
                break;
            default:
                break;
            }
            if(ingnore) {
                LOG_INFO(tag_, "Ingnore messege: " << message_type);
                continue;
            }
            // Determine client host name
            char client_host_name[MAX_HOSTNAME_LENGTH] = {0};
            GetRequestClientName(options, options_size, client_host_name, sizeof(client_host_name)-1);
            if (('\0' != server_host_name_[0]) && (0 == _stricmp(client_host_name, server_host_name_)))
            {
                LOG_INFO(tag_, "It is the server request.");
                continue;
            }

            unsigned int identifier_size;
            const BYTE* identifier_data = GetIdentifier(request, options, 
                                          options_size, &identifier_size);
            if(!identifier_data) {
                LOG_INFO(tag_, "GetIdentifier error! ");
                continue;
            }
                
            
            DWORD previous_offer_addr;
            auto has_client_before = CheckIsClientExist(identifier_data,
                                          identifier_size, &previous_offer_addr);

            int reply_size = sizeof(DHCPMessage) + sizeof(DHCPServerOptions);
            BYTE reply_message_buffer[sizeof(DHCPMessage) + sizeof(DHCPServerOptions)] = {0};

            const auto dhcp_reply = reinterpret_cast<DHCPMessage*>(&reply_message_buffer);
            CreateDhcpReplyPacket(request, dhcp_reply);
            DHCPServerOptions* const dhcp_reply_options = PacketOptions(dhcp_reply);
            if (dhcp_reply_options == nullptr) {
                LOG_INFO(tag_, "PacketOptions error! ");
                continue;
            }

            auto send_flag = false;
            switch (message_type) {
            case DHCPMessageType_DISCOVER: {
                LOG_INFO(tag_, "Receive Discover from client (" << client_host_name << ")");
                if (has_client_before)
                {
                    LOG_INFO(tag_, "The client is connected before");
                    send_flag = PacketOldClientOffer(dhcp_reply, dhcp_reply_options,
                                                        previous_offer_addr);
                }
                else
                {
                    send_flag = PacketNewClientOffer(dhcp_reply, dhcp_reply_options,
                                                        identifier_data, identifier_size);
                }
            }
            break;
            case DHCPMessageType_REQUEST: {
                LOG_INFO(tag_, "Receive Request from client (" << client_host_name << ")");
                send_flag = PacketRequestAck(dhcp_reply, dhcp_reply_options, request,
                              options, options_size, has_client_before, previous_offer_addr);
            }
            break;
            default:
                break;
            }

            if (send_flag) {
                SendDhcpMessage(request, dhcp_reply, dhcp_reply_options, reply_size);

                switch (dhcp_reply_options->pbMessageType[2]) {
                case DHCPMessageType_OFFER:
                    LOG_INFO(tag_, "Send offer to client (" << client_host_name << "), assign ip: " << DWIP0(dhcp_reply->yiaddr) << "." \
                        << DWIP1(dhcp_reply->yiaddr) << "." << DWIP2(dhcp_reply->yiaddr) << "." << DWIP3(dhcp_reply->yiaddr));
                    break;
                case DHCPMessageType_ACK:
                    LOG_INFO(tag_, "Send ack to client (" << client_host_name << ")");
                    break;
                default: 
                    break;
                }
            } 
        }
    }

    DHCPMessage* DhcpServer::ParseReceiveData(char* data, uint32_t len)
    {
        if (!data || len < sizeof(DHCPMessage))
            return nullptr;

        const auto dhcp_message = reinterpret_cast<DHCPMessage*>(data);
        if (!IsDhcpRequest(dhcp_message, len))
        {
            return nullptr;
        }
        return dhcp_message;
    }

    const BYTE* DhcpServer::GetOptions(const DHCPMessage* const message, uint32_t size, uint32_t* option_size)
    {
        if (!option_size || !message || size == 0)
            return nullptr;

        const BYTE* options = message->options + sizeof(pbDHCPMagicCookie);
        *option_size = size - sizeof(DHCPMessage) - sizeof(pbDHCPMagicCookie);
        return options;
    }

    const BYTE* DhcpServer::GetIdentifier(const DHCPMessage* const message, 
        const BYTE* options, uint32_t size, uint32_t* identifier_size)
    {
        if (!options || !identifier_size || size == 0)
            return nullptr;

        const BYTE* client_identifier;
        if (!FindOptionData(option_CLIENTIDENTIFIER, options,
            size, &client_identifier, identifier_size))
        {
            client_identifier = message->chaddr;
            *identifier_size = sizeof(message->chaddr);
        }
        return client_identifier;
    }

    bool DhcpServer::InitServer()
    {
        bool ret = false;
		LOG_INFO(tag_, "Dhcp Server init ... ");
        // Determine server hostname
        if (0 != gethostname(server_host_name_, 256))
        {
            server_host_name_[0] = '\0';
		}
       
        AddressInUseInformation sever_address;
        try {
            Poco::Net::SocketAddress address(net_address_.address, DHCP_SERVER_PORT);
            dhcp_socket_.reset(new Poco::Net::DatagramSocket(address));
            dhcp_socket_->setBroadcast(true);
            ret = true;
        }
	    catch (Poco::Exception& e) {
            LOG_INFO(tag_, "create socket error: " << e.code());
            ret = false;
        }
        if(ret) {
            sever_address.dwAddrValue = DWIPtoValue(server_address_.address);
            sever_address.pbClientIdentifier = nullptr;  // Server entry is only entry without a client ID
            sever_address.dwClientIdentifierSize = 0;
            addresses_in_use_.push_back(sever_address);
        }

        return ret;
    }

	bool DhcpServer::IsDhcpRequest(const DHCPMessage* const dhcp_request, const int data_size)
	{
		if (dhcp_request 
			&& (((sizeof(*dhcp_request) + sizeof(pbDHCPMagicCookie)) <= data_size)  // Take into account mandatory DHCP magic cookie values in options array (RFC 2131 section 3)
			&& (op_BOOTREQUEST == dhcp_request->op)
			&& (0 == memcmp(pbDHCPMagicCookie, dhcp_request->options, sizeof(pbDHCPMagicCookie))))
			)
		{
			return true;
		}
		return false;
	}

	
    bool DhcpServer::GetDHCPMessageType(const BYTE* const options, const int size, DHCPMessageTypes* const pdhcp_message_type)
    {
        bool ret = false;
        const BYTE* message_type_data;
        unsigned int data_size;
        if (FindOptionData(option_DHCPMESSAGETYPE, options, size, &message_type_data, &data_size) &&
            (1 == data_size) && (1 <= *message_type_data) && (*message_type_data <= 8))
        {
			*pdhcp_message_type = (DHCPMessageTypes)(*message_type_data);
			ret = true;	
        }
        return ret;
    }

	bool DhcpServer::GetRequestClientName(const BYTE* const options, const int size, char *client_name, const int len)
	{
        bool ret = false;
		const BYTE* request_host_name_data;
		unsigned int request_host_name_data_size;
		if (FindOptionData(option_HOSTNAME, options, size, &request_host_name_data, &request_host_name_data_size))
		{
			const size_t copy_size = min(request_host_name_data_size + 1, len);
			memcpy(client_name, request_host_name_data, copy_size);
            ret = true;
		}
		return ret;
	}

	bool DhcpServer::CheckIsClientExist(const BYTE* client_identifier_data, unsigned int client_identifier_data_size, DWORD *previous_offer_addr)
	{
        auto ret = false;
        if (!previous_offer_addr)
            return ret;

        auto address = static_cast<DWORD>(INADDR_BROADCAST);  // Invalid IP address for later comparison
		const ClientIdentifierData cid = { client_identifier_data, static_cast<DWORD>(client_identifier_data_size) };

		for (auto it : addresses_in_use_)
		{
			if (it.dwClientIdentifierSize != 0
				&& cid.dwClientIdentifierSize == it.dwClientIdentifierSize
				&& (0 == memcmp(cid.pbClientIdentifier, it.pbClientIdentifier, cid.dwClientIdentifierSize))
				)
			{
				address = DWValuetoIP(it.dwAddrValue);
				ret = true;
				break;
			}
		}

		*previous_offer_addr = address;
		return ret;
	}

    bool DhcpServer::FindOptionData(const BYTE find_option_value, const BYTE* const options, const int options_size, 
		const BYTE** const option_data, unsigned int* const option_data_size)
    {
        bool ret = false;
        // RFC 2132
        bool b_hit_end = false;
        const BYTE* current_options = options;
		if (find_option_value >= option_END)
			return ret;

		if (options && options_size > 2)
		{
			int i = 0;
			while ((i < options_size))
			{
                const BYTE current_option_value = options[i++];
				if (option_PAD == current_option_value)
				{
					continue;
				}
				else if (current_option_value >= option_END)
				{
					break;
				}
				else if(find_option_value == current_option_value && i < options_size)
				{
					int len = options[i++];
					const BYTE * data = options + i;
					*option_data = data;
					*option_data_size = len;
					ret = true;
					break;
				}
				else if(i < options_size)
				{
					int len = options[i++];
					i += len;
				}
			}
		}
        return ret;
    }

	bool DhcpServer::CreateDhcpReplyPacket(const DHCPMessage* const dhcp_request, DHCPMessage* const dhcp_reply) const
    {
		bool ret = false;
		if (dhcp_request && dhcp_reply)
		{
			dhcp_reply->op = op_BOOTREPLY;
			dhcp_reply->htype = dhcp_request->htype;
			dhcp_reply->hlen = dhcp_request->hlen;
			dhcp_reply->xid = dhcp_request->xid;

			dhcp_reply->flags = dhcp_request->flags;
			dhcp_reply->giaddr = dhcp_request->giaddr;
			CopyMemory(dhcp_reply->chaddr, dhcp_request->chaddr, sizeof(dhcp_reply->chaddr));
			strncpy_s(reinterpret_cast<char*>(dhcp_reply->sname), sizeof(dhcp_reply->sname), server_host_name_, _TRUNCATE);
			ret = true;
		}
		return ret;
	}

	DHCPServerOptions* DhcpServer::PacketOptions(DHCPMessage* pdhcpmReply) const
    {
		if (pdhcpmReply)
		{
            auto* pdhcpsoServerOptions = (DHCPServerOptions*)(pdhcpmReply->options);
			CopyMemory(pdhcpsoServerOptions->pbMagicCookie, pbDHCPMagicCookie, 
                               sizeof(pdhcpsoServerOptions->pbMagicCookie));
			// DHCP Message Type - RFC 2132 section 9.6
			pdhcpsoServerOptions->pbMessageType[0] = option_DHCPMESSAGETYPE;
			pdhcpsoServerOptions->pbMessageType[1] = 1;
	
			// IP Address Lease Time - RFC 2132 section 9.2
			pdhcpsoServerOptions->pbLeaseTime[0] = option_IPADDRESSLEASETIME;
			pdhcpsoServerOptions->pbLeaseTime[1] = 4;
			C_ASSERT(sizeof(u_long) == 4);
			*reinterpret_cast<u_long*>(&(pdhcpsoServerOptions->pbLeaseTime[2])) = 
                                      htonl(1 * 60 * 60 * 24 * 12 * 24 * 365);  // One year
			// Subnet Mask - RFC 2132 section 3.3
			pdhcpsoServerOptions->pbSubnetMask[0] = option_SUBNETMASK;
			pdhcpsoServerOptions->pbSubnetMask[1] = 4;
			C_ASSERT(sizeof(u_long) == 4);
			*reinterpret_cast<u_long*>(&(pdhcpsoServerOptions->pbSubnetMask[2])) = 
                                        server_address_.mask;

			pdhcpsoServerOptions->pbDNS[0] = option_DNS;
			pdhcpsoServerOptions->pbDNS[1] = 8;
			C_ASSERT(sizeof(u_long) == 4);
			*reinterpret_cast<u_long*>(&(pdhcpsoServerOptions->pbDNS[2])) = 
                                        server_address_.dns;
			*reinterpret_cast<u_long*>(&(pdhcpsoServerOptions->pbDNS[6])) = 
                                        server_address_.dns2;

			// Server Identifier - RFC 2132 section 9.7
			pdhcpsoServerOptions->pbServerID[0] = option_SERVERIDENTIFIER;
			pdhcpsoServerOptions->pbServerID[1] = 4;
			C_ASSERT(sizeof(u_long) == 4);
			*reinterpret_cast<u_long*>(&(pdhcpsoServerOptions->pbServerID[2])) = 
                                         server_address_.address; 
			pdhcpsoServerOptions->bEND = option_END;
			return pdhcpsoServerOptions;
		}

		return nullptr;
	}

	int DhcpServer::FindAddrIndex(DWORD addr_value)
	{
		int index = -1;
		bool found = false;
		for (auto it : addresses_in_use_)
		{
			index++;
			if (it.dwAddrValue == addr_value)
			{
				found = true;
				break;
			}
		}
		if (!found)
			index = -1;
		return index;
	}

	DWORD DhcpServer::AssignAddress()
	{
		const DWORD min_addr_value = DWIPtoValue(server_address_.min_address);
		const DWORD max_addr_value = DWIPtoValue(server_address_.max_address);
		int check_addr_count = 0;
		DWORD offer_addr_value = last_offer_address_ + 1;
		bool addr_value_valid = false;

		do {
			if (offer_addr_value > max_addr_value)
				offer_addr_value = min_addr_value;

			addr_value_valid = (FindAddrIndex(offer_addr_value) == -1)
				&& (offer_addr_value >= min_addr_value && offer_addr_value <= max_addr_value);

			check_addr_count++;
			if (!addr_value_valid)
				offer_addr_value++;
		} while (!addr_value_valid && (check_addr_count < (max_addr_value - min_addr_value)));

		if(addr_value_valid)
            last_offer_address_ = offer_addr_value;
		else
			offer_addr_value = 0;
		
		return offer_addr_value;
	}

    void DhcpServer::SetServerAddress(const DhcpNetWorkAddr net_address)
    {
        server_address_.address = DWIPtoValue(ntohl(inet_addr(net_address.address.c_str())));
        server_address_.mask = DWIPtoValue(ntohl(inet_addr(net_address.mask.c_str())));
        server_address_.min_address = DWIPtoValue(ntohl(inet_addr(net_address.min_addr.c_str())));
        server_address_.max_address = DWIPtoValue(ntohl(inet_addr(net_address.max_addr.c_str())));
        server_address_.dns = DWIPtoValue(ntohl(inet_addr(net_address.dns.c_str())));
        server_address_.dns2 = DWIPtoValue(ntohl(inet_addr(net_address.dns2.c_str())));
    }

    bool DhcpServer::PacketOldClientOffer(DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options, DWORD previous_offer_addr)
	{
		bool ret = false;
        const DWORD offer_addr_value = DWIPtoValue(previous_offer_addr);
		if (offer_addr_value != 0 && dhcp_reply && dhcp_reply_options)
		{
			const DWORD offer_addr = DWValuetoIP(offer_addr_value);
			dhcp_reply->yiaddr = offer_addr;
			dhcp_reply_options->pbMessageType[2] = DHCPMessageType_OFFER;
			ret = true;
		}
		return ret;
	}

	bool DhcpServer::PacketNewClientOffer(DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options,
		                             const BYTE* request_client_identifier, const unsigned int request_client_identifier_size)
	{
		bool ret = false;
        const DWORD offer_addr_value = AssignAddress();
		if (offer_addr_value != 0 && dhcp_reply && dhcp_reply_options)
		{
			AddressInUseInformation client_address;
			const DWORD offer_addr = DWValuetoIP(offer_addr_value);
			client_address.dwAddrValue = offer_addr_value;
			client_address.pbClientIdentifier = (BYTE*)LocalAlloc(LMEM_FIXED, request_client_identifier_size);
			if (0 != client_address.pbClientIdentifier)
			{
				CopyMemory(client_address.pbClientIdentifier, request_client_identifier, request_client_identifier_size);
				client_address.dwClientIdentifierSize = request_client_identifier_size;
				addresses_in_use_.push_back(client_address);
				{
					dhcp_reply->yiaddr = offer_addr;
					dhcp_reply_options->pbMessageType[2] = DHCPMessageType_OFFER;
					ret = true;
				}
			}
			else
			{
				LOG_INFO(tag_, "Insufficient memory to add client address.");
			}
		}
		return ret;
	}

	bool DhcpServer::PacketRequestAck(DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options,
		                          const DHCPMessage* const dhcp_request,
		                          const BYTE* const options, const int options_size, 
		                          const bool has_client_before, DWORD previous_offer_addr) const
    {
		bool ret = false;
		DWORD request_ip_address = INADDR_BROADCAST;  // Invalid IP address for later comparison
		const BYTE* requested_ip_address_data = 0;
		unsigned int requested_ip_address_data_size = 0;
		if (FindOptionData(option_REQUESTEDIPADDRESS, options, options_size, &requested_ip_address_data, &requested_ip_address_data_size)
			&& (sizeof(request_ip_address) == requested_ip_address_data_size))
		{
			request_ip_address = *((DWORD*)requested_ip_address_data);
		}

		const BYTE* request_server_Identifier_data = 0;
		unsigned int request_server_Identifier_data_size = 0;
		if (FindOptionData(option_SERVERIDENTIFIER, options, options_size, &request_server_Identifier_data, &request_server_Identifier_data_size) &&
			(sizeof(server_address_.address) == request_server_Identifier_data_size) && (server_address_.address == *((DWORD*)request_server_Identifier_data)))
		{
			// Response to OFFER
			if (has_client_before)
				dhcp_reply_options->pbMessageType[2] = DHCPMessageType_ACK;
			else
				dhcp_reply_options->pbMessageType[2] = DHCPMessageType_NAK;// Haven't seen this client before - NAK it
		}
		else
		{
			// Request to verify or extend
			if (((INADDR_BROADCAST != request_ip_address) /*&& (0 == pdhcpmRequest->ciaddr)*/) ||  // DHCPREQUEST generated during INIT-REBOOT state - Some clients set ciaddr in this case, so deviate from the spec by allowing it
				((INADDR_BROADCAST == request_ip_address) && (0 != dhcp_request->ciaddr)))  // Unicast -> DHCPREQUEST generated during RENEWING state / Broadcast -> DHCPREQUEST generated during REBINDING state
			{
				if (has_client_before && ((previous_offer_addr == request_ip_address) || (previous_offer_addr == dhcp_request->ciaddr)))
					// Already have an IP address for this client - ACK it
					dhcp_reply_options->pbMessageType[2] = DHCPMessageType_ACK;
				else
					// Haven't seen this client before or requested IP address is invalid
					dhcp_reply_options->pbMessageType[2] = DHCPMessageType_NAK;
			}
			else
			{
				LOG_INFO(tag_, "Invalid DHCP message (invalid data).");
			}
		}

		switch (dhcp_reply_options->pbMessageType[2])
		{
		case DHCPMessageType_ACK:
			// ASSERT(INADDR_BROADCAST != dwClientPreviousOfferAddr);
			dhcp_reply->ciaddr = previous_offer_addr;
			dhcp_reply->yiaddr = previous_offer_addr;
			ret = true;
			break;
		case DHCPMessageType_NAK:
			C_ASSERT(0 == option_PAD);
			ZeroMemory(dhcp_reply_options->pbLeaseTime, sizeof(dhcp_reply_options->pbLeaseTime));
			ZeroMemory(dhcp_reply_options->pbSubnetMask, sizeof(dhcp_reply_options->pbSubnetMask));
			ret = true;
			break;
		default:
			// Nothing to do
			break;
		}


		return ret;
	}


	void DhcpServer::SendDhcpMessage(const DHCPMessage* const dhcp_request, DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options, int reply_buffer_size) const
    {
		u_long ulAddr = INADDR_LOOPBACK;  // Invalid value
		if (!dhcp_request || !dhcp_reply || !dhcp_reply_options)
			return;

		if (0 == dhcp_request->giaddr)
		{
			switch (dhcp_reply_options->pbMessageType[2])
			{
			case DHCPMessageType_OFFER:
				// Fall-through
			case DHCPMessageType_ACK:
			{
				if (0 == dhcp_request->ciaddr)
				{
					if (0 != (BROADCAST_FLAG & dhcp_request->flags))
					{
						ulAddr = INADDR_BROADCAST;
					}
					else
					{
						ulAddr = dhcp_request->yiaddr;  // Already in network order
						if (0 == ulAddr)
						{
							// UNSUPPORTED: Unicast to hardware address
							// Instead, broadcast the response and rely on other DHCP clients to ignore it
							ulAddr = INADDR_BROADCAST;
						}
					}
				}
				else
				{
					ulAddr = dhcp_request->ciaddr;  // Already in network order
				}
			}
			break;
			case DHCPMessageType_NAK:
			{
				ulAddr = INADDR_BROADCAST;
			}
			break;
			default:
				break;
			}
		}
		else
		{
			ulAddr = dhcp_request->giaddr;  // Already in network order
			dhcp_reply->flags |= BROADCAST_FLAG;  // Indicate to the relay agent that it must broadcast
		}
        std::stringstream ss;
        ss << DWIP0(ulAddr) << "." \
            << DWIP1(ulAddr) << "." << DWIP2(ulAddr) << "." << DWIP3(ulAddr);
        LOG_INFO(tag_, "send to: " << ss.str());
        Poco::Net::SocketAddress address(ss.str(), DHCP_CLIENT_PORT);

		dhcp_socket_->sendTo(static_cast<void*>(dhcp_reply), reply_buffer_size, address);
	}
}
