#ifndef LOG_H
#define LOG_H
#include <string>
#include <sstream>

#if _DEBUG
#define LOG_INFO(TAG, SS) {std::stringstream str;\
                           str << "[" << TAG << "]"<<SS; \
                           dhcpserver::LOG::Info(str);}
#else
#define LOG_INFO(TAG, SS) {std::stringstream str;\
                               str << "[" << TAG << "]"<<SS; \
                               dhcpserver::LOG::Info(str);}
#endif

namespace dhcpserver
{

	class LOG
	{
	public:
		static void SetPath(std::string path);
		static void Info(std::stringstream &ss);
	};
}

#endif // THREAD_H
