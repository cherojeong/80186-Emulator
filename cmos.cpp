#include "devices.h"
#include "system.h"
#include <ctime>

namespace Devices {

	uint8_t active_register = 0;

	/* port 71h */

	uint16_t get(uint16_t port)
	{
		time_t t = time(0);   // get time now
		struct tm * now = localtime(&t);

		switch (active_register) {
		case 0:
			return now->tm_sec;
		case 2:
			return now->tm_min;
		case 4:
			return now->tm_hour;
		case 6:
			return now->tm_wday;
		case 7:
			return now->tm_mday;
		case 8:
			return now->tm_mon;
		case 9:
			return now->tm_year;
		default:
			return 0;
		}
	}

	uint8_t io_read_8(uint16_t port)
	{
		return (uint8_t)get(port);
	}

	uint16_t io_read_16(uint16_t port)
	{
		return get(port);
	}

	/* port 70h */

	void io_write_8(uint16_t port, uint8_t value)
	{
		active_register = value;
	}

	void create_cmos()
	{
		System::register_io_port(0x70, nullptr, nullptr, (void*)io_write_8, nullptr);
		System::register_io_port(0x71, (void*)io_read_8, (void*)io_read_16, nullptr, nullptr);
	}
}