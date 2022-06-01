#include "log.h"
#include <fstream>
#if DEBUG
#include <iostream>
#endif

namespace dhcpserver
{
	static std::string path_ = "F:\\Mindlinker\\logs\\dhcp.log";

	void LOG::SetPath(std::string path)
	{
		path_ = path;
	}
	void LOG::Info(std::stringstream &ss)
	{
		std::fstream file;
		file.open(path_, std::ios::out | std::ios::app);
		file << ss.str() << std::endl;
		file.close();
#if 1//_DEBUG
		std::cout << ss.str() <<std::endl;
#endif
	}
}
