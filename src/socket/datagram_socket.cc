#include "datagram_socket.h"

namespace poedhcpserver
{
#ifdef WIN32
    typedef int socklen_t;
#endif

    DatagramSocket::DatagramSocket(int port, char* address, bool broadcast, bool reusesock):
		 port_(port), broadcast_(broadcast), reusesock_(reusesock)
    {
        //set up bind address
        memset(&bindaddr_, 0, sizeof(bindaddr_));
		bindaddr_.sin_family = AF_INET;
		bindaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
		bindaddr_.sin_port = htons(port);

        //set up address to use for sending
        memset(&outaddr_, 0, sizeof(outaddr_));
        outaddr_.sin_family = AF_INET;
        outaddr_.sin_addr.s_addr = inet_addr(address);
        outaddr_.sin_port = htons(port);

    }

    DatagramSocket::~DatagramSocket()
    {
		disconnect();
    }

	int DatagramSocket::connect()
	{
#ifdef WIN32
		retval_ = WSAStartup(MAKEWORD(2, 2), &wsa_data_);
#endif

		sockaddr_in addr;
		sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (sock_ == INVALID_SOCKET)
		{
			connect_status_ = SOCKET_ERR;
			return connect_status_;
		}


#ifdef WIN32
		bool bOptVal = 1;
		int bOptLen = sizeof(bool);
#else
		int OptVal = 1;
#endif

		if (broadcast_)
#ifdef WIN32
			retval_ = setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, bOptLen);
#else
			retval_ = setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &OptVal, sizeof(OptVal));
#endif

		if (reusesock_)
#ifdef WIN32
			retval_ = setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (char*)&bOptVal, bOptLen);
#else
			retval_ = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &OptVal, sizeof(OptVal));
#endif

		retval_ = bind(sock_, (struct sockaddr *)&outaddr_, sizeof(outaddr_));
		if (retval_ == SOCKET_ERROR)
		{
			connect_status_ = BIND_ERR;
			return connect_status_;
		}

		connect_status_ = SUCCESS;
		return connect_status_;
	}

	int DatagramSocket::disconnect()
	{
#if WIN32
		closesocket(sock_);
		WSACleanup();
#else
		close(sock);
#endif
		return connect_status_;
	}

    int DatagramSocket::GetAddress(const char * name, char * addr)
    {
        struct hostent *hp;
        if ((hp = gethostbyname(name)) == NULL) return (0);
        strcpy(addr, inet_ntoa(*(struct in_addr*)(hp->h_addr)));
        return (1);
    }

    const char* DatagramSocket::GetAddress(const char * name)
    {
        struct hostent *hp;
        if ((hp = gethostbyname(name)) == NULL) return (0);
        strcpy(ip_, inet_ntoa(*(struct in_addr*)(hp->h_addr)));
        return ip_;
    }

    long DatagramSocket::Receive(char* msg, int msgsize)
    {
        struct sockaddr_in sender;
        socklen_t sender_size = sizeof(sender);
        int retval_ = recvfrom(sock_, msg, msgsize, 0, (struct sockaddr *)&sender, &sender_size);
        strcpy(received_, inet_ntoa(sender.sin_addr));
        return retval_;
    }

    char* DatagramSocket::ReceivedFrom()
    {
        return received_;
    }

    long DatagramSocket::Send(const char* msg, int msgsize)
    {
        return sendto(sock_, msg, msgsize, 0, (struct sockaddr *)&outaddr_, sizeof(outaddr_));
    }

    long DatagramSocket::SendTo(const char* msg, int msgsize, const char* addr)
    {
        outaddr_.sin_addr.s_addr = inet_addr(addr);
        return sendto(sock_, msg, msgsize, 0, (struct sockaddr *)&outaddr_, sizeof(outaddr_));
    }

    long DatagramSocket::SendTo(const char* msg, int msgsize, sockaddr* addr, int tolen)
    {
        return sendto(sock_, msg, msgsize, 0, addr, tolen);
    }

    int DatagramSocket::ConnectStatus()
    {
        return connect_status_;
    }
}
