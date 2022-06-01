#ifndef DATAGRAMSOCKET_H_INCLUDED
#define DATAGRAMSOCKET_H_INCLUDED

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#endif

namespace poedhcpserver
{
    //Simple socket class for datagrams.  Platform independent between
    //unix and Windows.
	enum CONNECT_STATIS
	{
		SUCCESS = 0,
		SOCKET_ERR,
		BIND_ERR,
		SOCKET_CLOSE = 255
    };
    class DatagramSocket
    {

    public:
        DatagramSocket(int port, char* address, bool broadcast, bool reusesock);

        ~DatagramSocket();

		int connect();

		int disconnect();

        char* ReceivedFrom();

        long Receive(char* msg, int msgsize);

        long Send(const char* msg, int msgsize);

        long SendTo(const char* msg, int msgsize, const char* name);

        long SendTo(const char* msg, int msgsize, struct sockaddr *addr, int tolen);

        int GetAddress(const char * name, char * addr);

        const char* GetAddress(const char * name);

        int ConnectStatus();

    private:
#ifdef WIN32
        WSAData     wsa_data_;
        SOCKET      sock_;
#else
        int         sock_;
#endif
        long        retval_;
		sockaddr_in bindaddr_;
        sockaddr_in outaddr_;
        char        ip_[30];
        char        received_[30];
        CONNECT_STATIS connect_status_;
		int         port_;
		bool        broadcast_;
		bool        reusesock_;
    };
}

#endif // DATAGRAMSOCKET_H_INCLUDED
