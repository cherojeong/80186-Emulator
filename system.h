#pragma once

#include <stdint.h>

#define BIOS_INTERRUPT_HOOK

namespace System {

	extern uint8_t * ram_8;
	extern uint16_t * ram_16;

	static inline void ram_set_8(uint16_t seg, uint16_t off, uint8_t x)
	{
		System::ram_8[seg * 16 + off] = x;
	}
	void dump_ram(const char * filename);

	static inline void ram_set_16(uint16_t seg, uint16_t off, uint16_t x) 
	{
		System::ram_8[seg*16 + off] = (uint8_t)x;
		System::ram_8[seg*16 + off + 1] = (uint8_t)(x >> 8);
	}

	#define ram_get_8(seg,off) (System::ram_8[seg*16 + off])
	#define ram_get_16(seg,off) ((uint16_t)(System::ram_8[seg*16 + off] | (System::ram_8[seg*16 + off + 1] << 8)))

	int create();

	// Returns 0: CPU should iret. 1: waiting on user input. 2: System Reset
	int bios_interrupt(uint8_t n, uint16_t * registers, uint16_t * segment_registers, uint16_t * FLAGS);

	void destroy();

	uint8_t in_8(uint16_t port);
	uint16_t in_16(uint16_t port);
	void out_8(uint16_t port, uint8_t value);
	void out_16(uint16_t port, uint16_t value);

	void register_io_port(uint16_t port, void *, void *, void *, void *);
	
};


