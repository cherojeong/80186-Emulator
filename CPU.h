#pragma once

#include <stdint.h>

namespace CPU {

	void create();

	/* Returns 1 if the cpu has halted, 2 if halted with interrupts disabled,
	3 if waiting for user input for a hooked bios interrupt */
	int step();

	void dump();
	void interrupt(uint8_t n);

};
