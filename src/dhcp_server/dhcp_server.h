#ifndef HDCP_SERVER_H
#define HDCP_SERVER_H
#include <vector>
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Thread.h"
namespace dhcpserver
{
	struct DhcpNetWorkAddr {
		std::string address;
		std::string mask;
		std::string min_addr;
		std::string max_addr;
		std::string dns;
		std::string dns2;
	};

    struct  DwordAddress
    {
        DWORD   address;
        DWORD   mask;
        DWORD   min_address;
        DWORD   max_address;
        DWORD   dns;
        DWORD   dns2;
    };

    struct AddressInUseInformation
    {
        DWORD dwAddrValue;
        BYTE* pbClientIdentifier;
        DWORD dwClientIdentifierSize;
        // SYSTEMTIME stExpireTime;  // If lease timeouts are needed
    };
    typedef std::vector<AddressInUseInformation> VectorAddressInUseInformation;

    struct DHCPMessage
    {
        BYTE op;
        BYTE htype;
        BYTE hlen;
        BYTE hops;
        DWORD xid;
        WORD secs;
        WORD flags;
        DWORD ciaddr;
        DWORD yiaddr;
        DWORD siaddr;
        DWORD giaddr;
        BYTE chaddr[16];
        BYTE sname[64];
        BYTE file[128];
        BYTE options[];
    };
    struct DHCPServerOptions
    {
        BYTE pbMagicCookie[4];
        BYTE pbMessageType[3];
        BYTE pbLeaseTime[6];
        BYTE pbSubnetMask[6];
		BYTE pbDNS[10];
        BYTE pbServerID[6];
        BYTE bEND;
    };

    enum op_values
    {
        op_BOOTREQUEST = 1,
        op_BOOTREPLY = 2,
    };

    // RFC 2132 section 9.6
    enum option_values
    {
        option_PAD = 0,
        option_SUBNETMASK = 1,
		option_ROUTER = 3, //length 4
		option_DNS = 6, //length 8
        option_HOSTNAME = 12,
        option_REQUESTEDIPADDRESS = 50,
        option_IPADDRESSLEASETIME = 51,
        option_DHCPMESSAGETYPE = 53,
        option_SERVERIDENTIFIER = 54,
        option_CLIENTIDENTIFIER = 61,
        option_END = 255,
    };
    enum DHCPMessageTypes
    {
		DHCPMessageType_NONE = 0,
        DHCPMessageType_DISCOVER = 1,
        DHCPMessageType_OFFER = 2,
        DHCPMessageType_REQUEST = 3,
        DHCPMessageType_DECLINE = 4,
        DHCPMessageType_ACK = 5,
        DHCPMessageType_NAK = 6,
        DHCPMessageType_RELEASE = 7,
        DHCPMessageType_INFORM = 8,
    };

    class DhcpServer : public Poco::Runnable
    {
    public:
        DhcpServer(DhcpNetWorkAddr net_address);

        ~DhcpServer();

        bool Start();

        bool Stop();

        bool IsRunning();

    private:
        void run() override;

        static DHCPMessage* ParseReceiveData(char *data, uint32_t len);

        static const BYTE* GetOptions(const DHCPMessage* const message, 
                     uint32_t size, uint32_t *option_size);

        const BYTE* GetIdentifier(const DHCPMessage* const message, 
            const BYTE* options, uint32_t size, uint32_t *identifier_size);

		void SendDhcpMessage(const DHCPMessage* const dhcp_request, DHCPMessage* const dhcp_reply,  DHCPServerOptions* const dhcp_reply_options, int reply_buffer_size) const;

        bool InitServer();

        static bool IsDhcpRequest(const DHCPMessage* const dhcp_request, const int data_size);

        static bool GetDHCPMessageType(const BYTE* const options, const int size, DHCPMessageTypes* const pdhcp_message_type);

        static bool GetRequestClientName(const BYTE* const options, const int size, char *client_name, const int len);

		bool CheckIsClientExist(const BYTE* client_identifier_data, unsigned int client_identifier_data_size, DWORD *previous_offer_addr);

		bool CreateDhcpReplyPacket(const DHCPMessage* const dhcp_request, DHCPMessage* const dhcp_reply) const;

        static bool FindOptionData(const BYTE find_option_value, const BYTE* const options, const int options_size, const BYTE** const option_data, unsigned int* const option_data_size);

        static bool PacketOldClientOffer(DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options, DWORD previous_offer_addr);

		bool PacketNewClientOffer(DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options,
                                    const BYTE* request_client_identifier, const unsigned int request_client_identifier_size);

		bool PacketRequestAck(DHCPMessage* const dhcp_reply, DHCPServerOptions* const dhcp_reply_options,
							const DHCPMessage* const dhcp_request,
							const BYTE* const options, const int options_size,
							const bool has_client_before, DWORD previous_offer_addr) const;

		DWORD AssignAddress();

        void SetServerAddress(const DhcpNetWorkAddr net_address);

		DHCPServerOptions* PacketOptions(DHCPMessage* pdhcpmReply) const;

		int FindAddrIndex(DWORD addr_value);
    private:
        std::unique_ptr<Poco::Net::DatagramSocket> dhcp_socket_;
        std::unique_ptr<Poco::Thread>   thread_;
		DhcpNetWorkAddr                 net_address_;
        DwordAddress                    server_address_;
        char                            server_host_name_[256];
        VectorAddressInUseInformation   addresses_in_use_;
		bool                            starting_;
		DWORD                           last_offer_address_;
		std::string                     tag_;
    };
}
#endif