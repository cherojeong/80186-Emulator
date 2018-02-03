// TODO implement rcl (0xd1 /2)

#include "system.h"
#include <iostream>

using namespace std;
using namespace System;

/* Sign extends an 8-bit integer to 16-bits */
#define sign_extend(x)(((x) & 0x80 ? (x | 0xff00) : (x)))

namespace CPU {
	void dump();

	/* Registers */

	uint16_t registers[8] = {};

#define AX registers[0]
#define CX registers[1]
#define DX registers[2]
#define BX registers[3]
#define SP registers[4]
#define BP registers[5]
#define SI registers[6]
#define DI registers[7]

	uint16_t FLAGS = 0xf000;

#define CF_ 0
#define CF (1<<0)
#define PF_ 2
#define PF (1<<2)
#define AF_ 4
#define AF (1<<4)
#define ZF_ 6
#define ZF (1<<6)
#define SF_ 7
#define SF (1<<7)
#define TF_ 8
#define TF (1<<8)
#define IF_ 9
#define IF (1<<9)
#define DF_ 10
#define DF (1<<10)
#define OF_ 11
#define OF (1<<11)

#ifdef BIOS_INTERRUPT_HOOK
	uint16_t IP = 0x7c00;
#else
	uint16_t IP = 0;
#endif

#ifdef BIOS_INTERRUPT_HOOK
	uint16_t segment_registers[8] = {0};
#else
	uint16_t segment_registers[8] = { 0, 0xffff };
#endif

#define ES segment_registers[0]
#define CS segment_registers[1]
#define SS segment_registers[2]
#define DS segment_registers[3]
#define FS segment_registers[4]
#define GS segment_registers[5]

	/* Interrupt Vector Table */

#ifdef _WIN32
#pragma pack(push,1)
#endif

	typedef struct IVTEntry
	{
		uint16_t offset;
		uint16_t segment;
	}
#ifndef _WIN32
	__attribute__((packed))
#endif
		IVTEntry;

#ifdef _WIN32
#pragma pack(pop)
#endif

	IVTEntry * ivt;

	/* Stack operations */

	static inline void push(uint16_t x)
	{
		SP -= 2;
		ram_set_16(SS, SP, x);
	}

	static inline uint16_t pop()
	{
		uint16_t x = ram_get_16(SS, SP);
		SP += 2;
		return x;
	}

	void interrupt(uint8_t n)
	{
		//cout << "Interrupt " << hex << (unsigned)n << ", AX = " << AX << dec << endl;
		push(FLAGS);
		push(CS);
		push(IP);
		FLAGS &= ~IF;
		CS = ivt[n].segment;
		IP = ivt[n].offset;
	}

	/* Used by many instructions for the 'rm16' operand */

	static uint16_t get_with_mode(uint16_t segment, int mode, int rm, int16_t& ipOffset, uint16_t * nextWord, bool lea = false)
	{
		ipOffset = 0;
		if (!mode) {
			uint16_t address = 0;
			switch (rm) {
			case 0:
				address = BX + SI;
				break;
			case 1:
				address = BX + DI;
				break;
			case 2:
				address = BP + SI;
				break;
			case 3:
				address = BP + DI;
				break;
			case 4:
				address = SI;
				break;
			case 5:
				address = DI;
				break;
			case 6:
				ipOffset = 2;
				address = ram_get_16(CS, IP);
				break;
			case 7:
				address = BX;
				break;
			}

			if (nextWord)
				*nextWord = ram_get_16(segment, address + 2);

			if(lea)
				return address;

			return ram_get_16(segment, address);
		}
		else if (mode == 1) {
			ipOffset = 1;
			uint16_t address = 0;
			switch (rm) {
			case 0:
				address = BX + SI;
				break;
			case 1:
				address = BX + DI;
				break;
			case 2:
				address = BP + SI;
				break;
			case 3:
				address = BP + DI;
				break;
			case 4:
				address = SI;
				break;
			case 5:
				address = DI;
				break;
			case 6:
				address = BP;
				break;
			case 7:
				address = BX;
				break;
			}
			address += (int8_t)ram_get_8(CS, IP);

			if (nextWord)
				*nextWord = ram_get_16(segment, address + 2);

			if(lea)
				return address;

			return ram_get_16(segment, address);
		}
		else if (mode == 2) {
			ipOffset = 2;
			uint16_t address = 0;
			switch (rm) {
			case 0:
				address = BX + SI;
				break;
			case 1:
				address = BX + DI;
				break;
			case 2:
				address = BP + SI;
				break;
			case 3:
				address = BP + DI;
				break;
			case 4:
				address = SI;
				break;
			case 5:
				address = DI;
				break;
			case 6:
				address = BP;
				break;
			case 7:
				address = BX;
				break;
			}
			address += (int16_t)ram_get_16(CS, IP);

			if (nextWord)
				*nextWord = ram_get_16(segment, address + 2);

			if(lea)
				return address;

			return ram_get_16(segment, address);
		}
		else {
			return registers[rm];
		}
	}

	/* Used by many instructions for the 'rm8' operand */

	static void set_with_mode(uint16_t segment, int mode, int rm, int x, int16_t& ipOffset)
	{
		ipOffset = 0;
		if (!mode) {
			if (x == -1) {
				if (rm == 6)
					x = ram_get_16(CS, IP + 2);
				else
					x = ram_get_16(CS, IP);
			}
			switch (rm) {
			case 0:
				ram_set_16(segment, BX + SI, (uint16_t)x);
				break;
			case 1:
				ram_set_16(segment, BX + DI, (uint16_t)x);
				break;
			case 2:
				ram_set_16(segment, BP + SI, (uint16_t)x);
				break;
			case 3:
				ram_set_16(segment, BP + DI, (uint16_t)x);
				break;
			case 4:
				ram_set_16(segment, SI, (uint16_t)x);
				break;
			case 5:
				ram_set_16(segment, DI, (uint16_t)x);
				break;
			case 6:
				ram_set_16(segment, ram_get_16(CS, IP), (uint16_t)x);
				ipOffset = 2;
				break;
			case 7:
				ram_set_16(segment, BX, (uint16_t)x);
				break;
			}
		}
		else if (mode == 1) {
			ipOffset = 1;
			if (x == -1)
				x = ram_get_16(CS, IP + 1);
			switch (rm) {
			case 0:
				ram_set_16(segment, BX + SI + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 1:
				ram_set_16(segment, BX + DI + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 2:
				ram_set_16(segment, BP + SI + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 3:
				ram_set_16(segment, BP + DI + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 4:
				ram_set_16(segment, SI + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 5:
				ram_set_16(segment, DI + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 6:
				ram_set_16(segment, BP + ram_get_8(CS, IP), (uint16_t)x);
				break;
			case 7:
				ram_set_16(segment, BX + ram_get_8(CS, IP), (uint16_t)x);
				break;
			}
		}
		else if (mode == 2) {
			ipOffset = 2;
			if (x == -1)
				x = ram_get_16(CS, IP + 2);
			switch (rm) {
			case 0:
			{
				ram_set_16(segment, BX + SI + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 1:
			{
				ram_set_16(segment, BX + DI + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 2:
			{
				ram_set_16(segment, BP + SI + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 3:
			{
				ram_set_16(segment, BP + DI + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 4:
			{
				ram_set_16(segment, SI + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 5:
			{
				ram_set_16(segment, DI + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 6:
			{
				ram_set_16(segment, BP + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
			case 7:
				ram_set_16(segment, BX + ram_get_16(CS, IP), (uint16_t)x);
				break;
			}
		}
		else {
			registers[rm] = (uint16_t)x;
		}
	}

	/* Used by many instructions for the 'rm8' operand */

	static uint8_t get_with_mode_8(uint16_t segment, int mode, int rm, int16_t& ipOffset)
	{
		ipOffset = 0;
		if (!mode) {
			switch (rm) {
			case 0:
				return ram_get_8(segment, BX + SI);
			case 1:
				return ram_get_8(segment, BX + DI);
			case 2:
				return ram_get_8(segment, BP + SI);
			case 3:
				return ram_get_8(segment, BP + DI);
			case 4:
				return ram_get_8(segment, SI);
			case 5:
				return ram_get_8(segment, DI);
			case 6:
				ipOffset = 2;
				return ram_get_8(segment, ram_get_16(CS, IP));
			case 7:
				return ram_get_8(segment, BX);
			}
		}
		else if (mode == 1) {
			ipOffset = 1;
			switch (rm) {
			case 0:
				return ram_get_8(segment, BX + SI + ram_get_8(CS, IP));
			case 1:
				return ram_get_8(segment, BX + DI + ram_get_8(CS, IP));
			case 2:
				return ram_get_8(segment, BP + SI + ram_get_8(CS, IP));
			case 3:
				return ram_get_8(segment, BP + DI + ram_get_8(CS, IP));
			case 4:
				return ram_get_8(segment, SI + ram_get_8(CS, IP));
			case 5:
				return ram_get_8(segment, DI + ram_get_8(CS, IP));
			case 6:
				return ram_get_8(segment, BP + ram_get_8(CS, IP));
			case 7:
				return ram_get_8(segment, BX + ram_get_8(CS, IP));
			}
		}
		else if (mode == 2) {
			ipOffset = 2;
			switch (rm) {
			case 0:
			{
				uint8_t x = ram_get_8(segment, BX + SI + ram_get_16(CS, IP));
				return x;
			}
			case 1:
			{
				uint8_t x = ram_get_8(segment, BX + DI + ram_get_16(CS, IP));
				return x;
			}
			case 2:
			{
				uint8_t x = ram_get_8(segment, BP + SI + ram_get_16(CS, IP));
				return x;
			}
			case 3:
			{
				uint8_t x = ram_get_8(segment, BP + DI + ram_get_16(CS, IP));
				return x;
			}
			case 4:
			{
				uint8_t x = ram_get_8(segment, SI + ram_get_16(CS, IP));
				return x;
			}
			case 5:
			{
				uint8_t x = ram_get_8(segment, DI + ram_get_16(CS, IP));
				return x;
			}
			case 6:
			{
				uint8_t x = ram_get_8(segment, BP + ram_get_16(CS, IP));
				return x;
			}
			case 7:
				uint8_t x = ram_get_8(segment, BX + ram_get_16(CS, IP));
				return x;
			}
		}
		else {
			if (rm < 4)
				return registers[rm] & 0xff;
			else
				return (uint8_t)(registers[rm - 4] >> 8);
		}
		return 0;
	}

	static void set_with_mode_8(uint16_t segment, int mode, int rm, int x, int16_t& ipOffset)
	{
		ipOffset = 0;
		if (!mode) {
			if (x == -1) {
				if (rm == 6)
					x = ram_get_8(CS, IP + 2);
				else
					x = ram_get_8(CS, IP);
			}
			switch (rm) {
			case 0:
				ram_set_8(segment, BX + SI, (uint8_t)x);
				break;
			case 1:
				ram_set_8(segment, BX + DI, (uint8_t)x);
				break;
			case 2:
				ram_set_8(segment, BP + SI, (uint8_t)x);
				break;
			case 3:
				ram_set_8(segment, BP + DI, (uint8_t)x);
				break;
			case 4:
				ram_set_8(segment, SI, (uint8_t)x);
				break;
			case 5:
				ram_set_8(segment, DI, (uint8_t)x);
				break;
			case 6:
				ram_set_8(segment, ram_get_16(CS, IP), (uint8_t)x);
				ipOffset = 2;
				break;
			case 7:
				ram_set_8(segment, BX, (uint8_t)x);
				break;
			}
		}
		else if (mode == 1) {
			ipOffset = 1;
			if (x == -1)
				x = ram_get_8(CS, IP + 1);
			switch (rm) {
			case 0:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, BX + SI + off, (uint8_t)x);
				break;
			}
			case 1:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, BX + DI + off, (uint8_t)x);
				break;
			}
			case 2:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, BP + SI + off, (uint8_t)x);
				break;
			}
			case 3:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, BP + DI + off, (uint8_t)x);
				break;
			}
			case 4:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, SI + off, (uint8_t)x);
				break;
			}
			case 5:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, DI + off, (uint8_t)x);
				break;
			}
			case 6:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, BP + off, (uint8_t)x);
				break;
			}
			case 7:
			{
				int8_t off = (int8_t)ram_get_8(CS, IP);
				ram_set_8(segment, BX + off, (uint8_t)x);
				break;
			}
			}
		}
		else if (mode == 2) {
			ipOffset = 2;
			if (x == -1)
				x = ram_get_8(CS, IP + 2);
			switch (rm) {
			case 0:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, BX + SI + off, (uint8_t)x);
				break;
			}
			case 1:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, BX + DI + off, (uint8_t)x);
				break;
			}
			case 2:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, BP + SI + off, (uint8_t)x);
				break;
			}
			case 3:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, BP + DI + off, (uint8_t)x);
				break;
			}
			case 4:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, SI + off, (uint8_t)x);
				break;
			}
			case 5:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, DI + off, (uint8_t)x);
				break;
			}
			case 6:
			{
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, BP + off, (uint8_t)x);
				break;
			}
			case 7:
				int16_t off = (int16_t)ram_get_16(CS, IP);
				ram_set_8(segment, BX + off, (uint8_t)x);
				break;
			}
		}
		else {
			if (rm < 4) {
				registers[rm] &= 0xff00;
				registers[rm] |= x;
			}
			else {
				registers[rm - 4] &= 0x00ff;
				registers[rm - 4] |= (x << 8);
			}
		}
	}

	/* Sets the parity flag in FLAGS register */

	void set_parity(uint8_t x)
	{
		int count = 0;
		if (x & 1) count++;
		if (x & 2) count++;
		if (x & 4) count++;
		if (x & 8) count++;
		if (x & 16) count++;
		if (x & 32) count++;
		if (x & 64) count++;
		if (x & 128) count++;

		if (!(count % 2))
			FLAGS |= PF;
		else
			FLAGS &= ~PF;
	}

	/* Instructions for different operations that require flags to be set depending on the result */

	uint16_t add_16(uint16_t dst, uint16_t src)
	{
		if (dst >= 32768 && src >= 32768)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		uint16_t result = dst + src;
		if (!(dst & 0x8000) && !(src & 0x8000) && (result & 0x8000))
			FLAGS |= OF;
		else if ((dst & 0x8000) && (src & 0x8000) && !(result & 0x8000))
			FLAGS |= OF;
		else
			FLAGS &= ~OF;
		set_parity((uint8_t)result);
		if (((dst & 0xf) + (src & 0xf)) & 0xf0)
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint8_t add_8(uint8_t dst, uint8_t src)
	{
		if (dst >= 128 && src >= 128)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		uint8_t result = dst + src;
		if (!(dst & 0x80) && !(src & 0x80) && (result & 0x80))
			FLAGS |= OF;
		else if ((dst & 0x80) && (src & 0x80) && !(result & 0x80))
			FLAGS |= OF;
		else
			FLAGS &= ~OF;
		set_parity(result);
		if (((dst & 0xf) + (src & 0xf)) & 0xf0)
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint16_t inc_16(uint16_t x)
	{
		uint16_t result = x + 1;
		set_parity((uint8_t)result);
		if (!(result & 0xf))
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result) {
			FLAGS &= ~ZF;
			FLAGS &= ~OF;
		}
		else {
			FLAGS |= ZF;
			FLAGS |= OF;
		}
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint8_t inc_8(uint8_t x)
	{
		uint8_t result = x + 1;
		set_parity(result);
		if (!(result & 0xf))
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result) {
			FLAGS &= ~ZF;
			FLAGS &= ~OF;
		}
		else {
			FLAGS |= ZF;
			FLAGS |= OF;
		}
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint16_t dec_16(uint16_t x)
	{
		uint16_t result = x - 1;
		set_parity((uint8_t)result);
		if ((result & 0xf) == 0xf)
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result) {
			FLAGS &= ~ZF;
		}
		else {
			FLAGS |= ZF;
		}
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;

		if (((x & 0x8000) && !(result & 0x8000)) || (!(x & 0x8000) && (result & 0x8000)))
			FLAGS |= OF;
		else
			FLAGS &= ~OF;
		return result;
	}

	uint8_t dec_8(uint8_t x)
	{
		uint8_t result = x - 1;
		set_parity(result);
		if ((result & 0xf) == 0xf)
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result) {
			FLAGS &= ~ZF;
		}
		else {
			FLAGS |= ZF;
		}
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;

		if (((x & 0x80) && !(result & 0x80)) || (!(x & 0x80) && (result & 0x80)))
			FLAGS |= OF;
		else
			FLAGS &= ~OF;
		return result;
	}

	uint16_t sub_16(uint16_t dst, uint16_t src)
	{
		if (dst < src)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		uint16_t result = dst - src;
		set_parity((uint8_t)result);
		if ((int)(dst & 0xf) - (int)(src & 0xf) < 0)
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;

		if (!(dst & 0x8000) && (src & 0x8000) && (result & 0x8000))
			FLAGS |= OF;
		else if ((dst & 0x8000) && !(src & 0x8000) && !(result & 0x8000))
			FLAGS |= OF;
		else
			FLAGS &= ~OF;

		return result;
	}

	uint8_t sub_8(uint8_t dst, uint8_t src)
	{
		if (dst < src)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		uint8_t result = dst - src;
		set_parity(result);
		if ((int)(dst & 0xf) - (int)(src & 0xf) < 0)
			FLAGS |= AF;
		else
			FLAGS &= ~AF;
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;

		if (!(dst & 0x80) && (src & 0x80) && (result & 0x80))
			FLAGS |= OF;
		else if ((dst & 0x80) && !(src & 0x80) && !(result & 0x80))
			FLAGS |= OF;
		else
			FLAGS &= ~OF;

		return result;
	}

	uint16_t and_16(uint16_t dst, uint16_t src)
	{
		FLAGS &= ~(OF | CF);
		uint16_t result = dst & src;
		set_parity((uint8_t)result);
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint8_t and_8(uint8_t dst, uint8_t src)
	{
		FLAGS &= ~(OF | CF);
		uint8_t result = dst & src;
		set_parity(result);
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint16_t xor_16(uint16_t dst, uint16_t src)
	{
		FLAGS &= ~(OF | CF);
		uint16_t result = dst ^ src;
		set_parity((uint8_t)result);
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint8_t xor_8(uint8_t dst, uint8_t src)
	{
		FLAGS &= ~(OF | CF);
		uint8_t result = dst ^ src;
		set_parity(result);
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint16_t or_16(uint16_t dst, uint16_t src)
	{
		FLAGS &= ~(OF | CF);
		uint16_t result = dst | src;
		set_parity((uint8_t)result);
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x8000)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint8_t or_8(uint8_t dst, uint8_t src)
	{
		FLAGS &= ~(OF | CF);
		uint8_t result = dst | src;
		set_parity(result);
		if (result)
			FLAGS &= ~ZF;
		else
			FLAGS |= ZF;
		if (result & 0x80)
			FLAGS |= SF;
		else
			FLAGS &= ~SF;
		return result;
	}

	uint8_t rol_8(uint8_t dst, uint8_t n)
	{
		uint8_t originalSignBit = dst >> 7;
		for (unsigned int i = 0; i < n; i++) {
			uint8_t msb = dst >> 7;
			dst = dst << 1;
			dst |= msb;
		}
		if (dst & 1)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 7) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint16_t rol_16(uint16_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 15;
		for (unsigned int i = 0; i < n; i++) {
			uint16_t msb = dst >> 15;
			dst = dst << 1;
			dst |= msb;
		}
		if (dst & 1)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 15) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint8_t shl_8(uint8_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 7;
		unsigned int lastMSB = 0;
		for (unsigned int i = 0; i < n; i++) {
			lastMSB = dst >> 7;
			dst = dst << 1;
		}
		if (lastMSB)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 7) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint16_t ror_16(uint16_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 15;
		for (unsigned int i = 0; i < n; i++) {
			uint16_t lsb = dst & 1;
			dst = dst >> 1;
			dst |= lsb;
		}
		if (dst >> 15)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 15) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint8_t ror_8(uint8_t dst, uint8_t n)
	{
		uint8_t originalSignBit = dst >> 7;
		for (unsigned int i = 0; i < n; i++) {
			uint8_t lsb = dst & 1;
			dst = dst >> 1;
			dst |= lsb;
		}
		if (dst >> 7)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 7) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint16_t shl_16(uint16_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 15;
		unsigned int lastMSB = 0;
		for (unsigned int i = 0; i < n; i++) {
			lastMSB = dst >> 15;
			dst = dst << 1;
		}
		if (lastMSB)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 15) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint16_t shr_16(uint16_t dst, uint8_t n)
	{
		unsigned int lastLSB = 0;
		for (unsigned int i = 0; i < n; i++) {
			lastLSB = dst & 1;
			dst = dst >> 1;
		}
		if (lastLSB)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 15))
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint8_t shr_8(uint8_t dst, uint8_t n)
	{
		unsigned int lastLSB = 0;
		for (unsigned int i = 0; i < n; i++) {
			lastLSB = dst & 1;
			dst = dst >> 1;
		}
		if (lastLSB)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 7))
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint8_t rcr_8(uint8_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 7;
		unsigned int cf = FLAGS & CF;
		for (unsigned int i = 0; i < n; i++) {
			unsigned int lsb = dst & 1;
			dst = dst >> 1;
			dst |= cf << 7;
			cf = lsb;
		}
		if (cf)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 7) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint16_t rcr_16(uint16_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 15;
		unsigned int cf = FLAGS & CF;
		for (unsigned int i = 0; i < n; i++) {
			unsigned int lsb = dst & 1;
			dst = dst >> 1;
			dst |= cf << 15;
			cf = lsb;
		}
		if (cf)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 15) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint16_t rcl_16(uint16_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 15;
		unsigned int cf = FLAGS & CF;
		for (unsigned int i = 0; i < n; i++) {
			unsigned int tmpcf = dst >> 15;
			dst = dst << 1 | cf;
			cf = tmpcf;
		}
		if (cf)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if (cf != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	uint8_t rcl_8(uint8_t dst, uint8_t n)
	{
		uint8_t originalSignBit = dst >> 7;
		unsigned int cf = FLAGS & CF;
		for (unsigned int i = 0; i < n; i++) {
			unsigned int tmpcf = dst >> 7;
			dst = dst << 1 | cf;
			cf = tmpcf;
		}
		if (cf)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if (cf != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	int8_t sar_8(int8_t dst, uint8_t n)
	{
		uint8_t originalSignBit = dst >> 7;
		int cf = 0;
		for (unsigned int i = 0; i < n; i++) {
			cf = dst & 1;
			dst = dst / 2;
		}
		if (cf)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 7) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	int16_t sar_16(int16_t dst, uint8_t n)
	{
		uint16_t originalSignBit = dst >> 15;
		int cf = 0;
		for (unsigned int i = 0; i < n; i++) {
			cf = dst & 1;
			dst = dst / 2;
		}
		if (cf)
			FLAGS |= CF;
		else
			FLAGS &= ~CF;
		if (n == 1) {
			if ((dst >> 15) != originalSignBit)
				FLAGS |= OF;
			else
				FLAGS &= ~OF;
		}
		return dst;
	}

	int32_t imul_16(int16_t dst, int16_t src)
	{
		int32_t result = (int16_t)(dst*src);

		if (result & 0xffff0000) {
			FLAGS |= CF | DF;
		}
		else {
			FLAGS &= ~(CF | DF);
		}

		return result;
	}

	int16_t imul_8(int8_t dst, int8_t src)
	{
		int16_t result = (int8_t)(dst*src);

		if (result & 0xff00) {
			FLAGS |= CF | DF;
		}
		else {
			FLAGS &= ~(CF | DF);
		}

		return result;
	}

	uint16_t sbb_16(uint16_t dst, uint16_t src)
	{
		return sub_16(dst, src + ((FLAGS & CF) >> CF_));
	}

	uint8_t sbb_8(uint8_t dst, uint8_t src)
	{
		return sub_8(dst, src + ((FLAGS & CF) >> CF_));
	}

#define REP_NONE 0
#define REP_CX_0 1
#define REP_CX_0_ZF_0 2
#define REP_CX_0_ZF_1 3

	int repeatState = REP_NONE;
	uint8_t op;
	uint16_t segment = 0;

	/* Implementation of instructions that can be prefixed with the 'REP' prefix */

	int repeatableop()
	{
		switch (op) {
		case 0x6c:
		{ /* insb */
			ram_set_8(ES, DI, in_8(DX));
			if (FLAGS & DF) {
				DI--;
			}
			else {
				DI++;
			}
			break;
		}

		case 0x6d:
		{ /* insw */
			ram_set_16(ES, DI, in_16(DX));
			if (FLAGS & DF) {
				DI -= 2;
			}
			else {
				DI += 2;
			}
			break;
		}

		case 0xa4:
		{ /* movsb */
			ram_set_8(ES, DI, ram_get_8(segment, SI));
			if (FLAGS & DF) {
				DI--;
				SI--;
			}
			else {
				DI++;
				SI++;
			}
			break;
		}

		case 0xa5:
		{ /* movsw */
			ram_set_16(ES, DI, ram_get_16(segment, SI));
			if (FLAGS & DF) {
				DI -= 2;
				SI -= 2;
			}
			else {
				DI += 2;
				SI += 2;
			}
			break;
		}

		case 0xa6:
		{ /* cmpsb */
			sub_8(ram_get_8(segment, SI), ram_get_8(ES, DI));
			if (FLAGS & DF) {
				SI--;
				DI--;
			}
			else {
				SI++;
				DI++;
			}
			break;
		}

		case 0xa7:
		{ /* cmpsw */
			sub_16(ram_get_16(segment, SI), ram_get_16(ES, DI));
			if (FLAGS & DF) {
				SI -= 2;
				DI -= 2;
			}
			else {
				SI += 2;
				DI += 2;
			}
			break;
		}

		case 0xaa:
		{ /* stosb */
			ram_set_8(ES, DI, (uint8_t)AX);
			if (FLAGS & DF) {
				DI--;
			}
			else {
				DI++;
			}
			break;
		}

		case 0xab:
		{ /* stosw */
			ram_set_16(ES, DI, AX);
			if (FLAGS & DF) {
				DI -= 2;
			}
			else {
				DI += 2;
			}
			break;
		}

		case 0xac:
		{ /* lodsb */
 			((uint8_t*)&AX)[0] = ram_get_8(segment, SI);
			if (FLAGS & DF) {
				SI--;
			}
			else {
				SI++;
			}
			break;
		}
		case 0xad:
		{ /* lodsw */
			AX = ram_get_16(segment, SI);
			if (FLAGS & DF) {
				SI -= 2;
			}
			else {
				SI += 2;
			}
			break;
		}

		case 0xae:
		{ /* scasb */
			sub_8((uint8_t)AX, ram_get_8(ES, DI));
			if (FLAGS & DF) {
				DI--;
			}
			else {
				DI++;
			}
			break;
		}

		case 0xaf:
		{ /* scasw */
			sub_16(AX, ram_get_16(ES, DI));
			if (FLAGS & DF) {
				DI -= 2;
			}
			else {
				DI += 2;
			}
			break;
		}

		default:
			cout << "Unrecognised op (with rep prefix): " << hex << (unsigned int)op << dec << endl;
			return 1;
		}
		return 0;
	}


	int step()
	{
#ifdef BIOS_INTERRUPT_HOOK

		if ((CS * 16 + IP) >= 0x9fC00 && (CS * 16 + IP) < (0x9fC00 + 256)) {
			int n = (CS * 16 + IP) - 0x9fC00;
			int doNotIret = bios_interrupt(n, registers, segment_registers, &FLAGS);
			if (!doNotIret) {
				/* iret */
				IP = pop();
				CS = pop();
				uint16_t flags = pop();
				FLAGS &= ~IF;
				FLAGS |= flags & IF;
				return 0;
			}
			return 3;
		}

#endif

		/* if the virtual CPU is executing an instruction with one of the repeat prefixes, */
		/* check if the terminating condition(s) have been met */
		
		if (repeatState == REP_CX_0) {
			if (!CX)
				repeatState = REP_NONE;
		}
		else if (repeatState == REP_CX_0_ZF_0) {
			if (!CX || !(FLAGS & ZF))
				repeatState = REP_NONE;
		}
		else if (repeatState == REP_CX_0_ZF_1) {
			if (!CX || (FLAGS & ZF))
				repeatState = REP_NONE;
		}

		if (repeatState) {
			int error = repeatableop();
			if (error) return 2;
			CX--;
			return 0;
		}

		segment = DS;
		
		bool rep = false;
		bool repne = false;

		/* prefixes */

		while (1) {
			op = ram_get_8(CS, IP++);

			if (op == 0x2e) {
				segment = CS;
			}
			else if (op == 0x3e) {
				segment = DS;
			}
			else if (op == 0x26) {
				segment = ES;
			}
			else if (op == 0x36) {
				segment = SS;
			}
			else if (op == 0x64) {
				segment = FS;
			}
			else if (op == 0x65) {
				segment = GS;
			}
			else if (op == 0xf2) {
				repne = true;
			}
			else if (op == 0xf3) {
				rep = true;
			}
			else {
				break;
			}
		}

		if (rep) {
			if (op == 0xa6 || op == 0xa7 ||
				op == 0xae || op == 0xaf) {
				repeatState = REP_CX_0_ZF_0;
			}
			else {
				repeatState = REP_CX_0;
			}

			if (CX) {
				int error = repeatableop();
				if (error) return 2;
				CX--;
			}
			else
				repeatState = REP_NONE;
			return 0;
		}
		else if (repne) {
			repeatState = REP_CX_0_ZF_1;

			if (CX) {
				int error = repeatableop();
				if (error) return 2;
				CX--;
			}
			else
				repeatState = REP_NONE;
			return 0;
		}

		//cout << "op 0x" << hex << (unsigned)op << ", CS=" << CS << ", IP=" << (IP-1) << ",SS=" << SS << dec << endl;
		
		switch (op) {
		case 0x00:
		{ /* add rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, add_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, add_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x01:
		{ /* add rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, add_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x02:
		{ /* add reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				((uint8_t*)&registers[reg])[0] = add_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			else
				((uint8_t*)&registers[reg - 4])[1] = add_8(registers[reg - 4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));
			IP += ipOffset;
			break;
		}

		case 0x03:
		{ /* add reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = add_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0));
			IP += ipOffset;
			break;
		}

		case 0x04:
		{ /* add al, imm8 */
			uint8_t imm8 = ram_get_8(CS, IP++);
			((uint8_t*)&AX)[0] = add_8((uint8_t)AX, imm8);
			break;
		}

		case 0x05:
		{ /* add ax, imm16 */
			uint16_t imm16 = ram_get_16(CS, IP);
			IP += 2;
			AX = add_16(AX, imm16);
			break;
		}

		case 0x06:
		{
			push(ES);
			break;
		}

		case 0x07:
		{
			ES = pop();
			break;
		}

		case 0x08:
		{ /* or rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, or_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, or_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x09:
		{ /* or rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, or_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x0a:
		{ /* or reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4) {
				((uint8_t*)&registers[reg])[0] = or_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			}
			else {
				((uint8_t*)&registers[reg - 4])[1] = or_8(registers[reg - 4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));
			}
			IP += ipOffset;
			break;
		}

		case 0x0b:
		{ /* or reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = or_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0));
			IP += ipOffset;
			break;
		}

		case 0x0c:
		{ /* or al, imm8 */
			uint8_t imm8 = ram_get_8(CS, IP++);
			((uint8_t*)(&AX))[0] = or_8((uint8_t)AX, imm8);
			break;
		}

		case 0x0d:
		{ /* or ax, imm16 */
			uint16_t imm16 = ram_get_16(CS, IP);
			IP += 2;
			AX = or_16(AX, imm16);
			break;
		}

		case 0x0e:
		{
			push(CS);
			break;
		}

		case 0x0f:
		{
			op = ram_get_8(CS, IP++);
			switch (op) {
			case 0x80:
				if (FLAGS & OF)
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x81:
				if (!(FLAGS & OF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x82:
				if (FLAGS & CF)
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x83:
				if (!(FLAGS & CF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x84:
				if (FLAGS & ZF)
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x85:
				if (!(FLAGS & ZF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x86:
				if ((FLAGS & CF) || (FLAGS & ZF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x87:
				if (!(FLAGS & CF) && !(FLAGS & ZF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x88:
				if (FLAGS & SF)
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x89:
				if (!(FLAGS & SF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x8a:
				if ((FLAGS & PF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x8b:
				if (!(FLAGS & PF))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x8c:
				if (((FLAGS & SF) && !(FLAGS & OF)) || (!(FLAGS & SF) && (FLAGS & OF)))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x8d:
				if (((FLAGS & SF) && (FLAGS & OF)) || (!(FLAGS & SF) && !(FLAGS & OF)))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x8e:
				if ((FLAGS & ZF) || ((FLAGS & SF) && !(FLAGS & OF)) || (!(FLAGS & SF) && (FLAGS & OF)))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			case 0x8f:
				if (!(FLAGS & ZF) && ((FLAGS & SF) >> SF_) == ((FLAGS & OF) >> OF_))
					IP += (int16_t)ram_get_16(CS, IP);
				IP += 2;
				break;
			default:
				cout << "Unrecognised op2 for 0x0f (0x" << hex << (unsigned)op << dec << "). May be pop cs or unimplemented opcode (TODO remove this message)" << endl;
				CS = pop();
				IP--;
				break;
			}
			break;
		}

		case 0x10:
		{ /* adc rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, add_8(get_with_mode_8(segment, mode, rm, ipOffset), (registers[reg] & 0xff) + (FLAGS & CF)), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, add_8(get_with_mode_8(segment, mode, rm, ipOffset), (registers[reg - 4] >> 8) + (FLAGS & CF)), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x11:
		{ /* adc rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, add_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg] + (FLAGS & CF)), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x12:
		{ /* adc reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				((uint8_t*)&registers[reg])[0] = add_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset) + (FLAGS & CF));
			else
				((uint8_t*)&registers[reg - 4])[1] = add_8(registers[reg - 4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset) + (FLAGS & CF));
			IP += ipOffset;
			break;
		}

		case 0x13:
		{ /* adc reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = add_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0) + (FLAGS & 1));
			IP += ipOffset;
			break;
		}

		case 0x14:
		{ /* adc al */
			uint8_t imm8 = ram_get_8(CS, IP++);
			((uint8_t*)&AX)[0] = add_8((uint8_t)AX, imm8 + (FLAGS & CF));
			break;
		}

		case 0x15:
		{ /* adc ax */
			uint16_t imm16 = ram_get_16(CS, IP);
			IP += 2;
			AX = add_16(AX, imm16 + (FLAGS & CF));
			break;
		}

		case 0x16:
		{
			push(SS);
			break;
		}

		case 0x17:
		{
			SS = pop();
			break;
		}

		case 0x18:
		{ /* sbb rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, sbb_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, sbb_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x19:
		{ /* sbb rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, sbb_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x1a:
		{ /* sbb reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				((uint8_t*)&registers[reg])[0] = sbb_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			else
				((uint8_t*)&registers[reg - 4])[1] = sbb_8(registers[reg-4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));
			IP += ipOffset;
			break;
		}

		case 0x1c:
		{ /* sbb al, imm8 */
			((uint8_t *)&AX)[0] = sbb_8((uint8_t)AX, ram_get_8(CS, IP++));
			break;
		}

		case 0x1d:
		{ /* and ax, imm16 */
			AX = sbb_16(AX, ram_get_16(CS, IP));
			IP += 2;
			break;
		}

		case 0x1e:
		{
			push(DS);
			break;
		}

		case 0x1f:
		{
			DS = pop();
			break;
		}

		case 0x20:
		{ /* and rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, and_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, and_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x21:
		{ /* and rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, and_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x22:
		{ /* and reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				((uint8_t*)&registers[reg])[0] = and_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			else
				((uint8_t*)&registers[reg - 4])[1] = and_8(registers[reg-4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));
			IP += ipOffset;
			break;
		}

		case 0x23:
		{ /* and reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = and_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0));
			IP += ipOffset;
			break;
		}

		case 0x24:
		{ /* and al, imm8 */
			((uint8_t *)&AX)[0] = and_8((uint8_t)AX, ram_get_8(CS, IP++));
			break;
		}

		case 0x25:
		{ /* and ax, imm16 */
			AX = and_16(AX, ram_get_16(CS, IP));
			IP += 2;
			break;
		}

		case 0x27:
		{ /* daa */
			uint8_t oldAL = (uint8_t)AX;
			uint16_t oldCF = (FLAGS & CF) >> CF_;

			if ((AX & 0xf) > 9 || (uint8_t)AX == 1) {
				uint16_t x = (uint8_t)AX;
				x += 6;
				((uint8_t*)&AX)[0] = (uint8_t)x;
				if (x > 255)
					FLAGS |= CF;
				FLAGS |= AF;
			}
			else FLAGS &= ~AF;

			if (oldAL > 0x99 || oldCF) {
				uint8_t x = (uint8_t)AX;
				x += 0x60;
				((uint8_t*)&AX)[0] = x;
				FLAGS |= CF;
			}
			else FLAGS &= ~CF;

			set_parity((uint8_t)AX);
			if (AX & 0xff) FLAGS &= ~ZF;
			else FLAGS |= ZF;

			if (AX & 0x8000) FLAGS |= SF;
			else FLAGS &= ~SF;
			break;
		}

		case 0x28:
		{ /* sub rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, sub_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, sub_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x29:
		{ /* sub rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, sub_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x2a:
		{ /* sub reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4) 
				((uint8_t*)&registers[reg])[0] = sub_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			else
				((uint8_t*)&registers[reg-4])[1] = sub_8(registers[reg] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));			
			IP += ipOffset;
			break;
		}

		case 0x2b:
		{ /* sub reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = sub_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0));
			IP += ipOffset;
			break;
		}

		case 0x2c:
		{ /* sub al, imm8 */
			((uint8_t *)&AX)[0] = sub_8((uint8_t)AX, ram_get_8(CS, IP++));
			break;
		}

		case 0x2d:
		{ /* sub ax, imm16 */
			AX = sub_16(AX, ram_get_16(CS, IP));
			IP += 2;
			break;
		}

		case 0x2f:
		{ /* das */
			uint8_t OldAL = (uint8_t)AX;
			uint8_t OldCF = (FLAGS & CF) >> CF_;
			FLAGS &= ~CF;

			if((AX & 0xF) > 9 || FLAGS & AF) {
				if((uint8_t)AX > 6) FLAGS |= CF;
				else FLAGS |= OldCF;
				FLAGS |= AF;
			}
			else FLAGS &= ~AF;

			if(OldAL > 0x99 || OldCF) {
				((uint8_t *)&AX)[0] = (uint8_t)AX - 0x60;
				FLAGS |= CF;
			}
			else FLAGS &= ~CF;
			break;
		}

		case 0x30:
		{ /* xor rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, xor_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff), ipOffset);
			else
				set_with_mode_8(segment, mode, rm, xor_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x31:
		{ /* xor rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, xor_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x32:
		{ /* xor reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				((uint8_t*)&registers[reg])[0] = xor_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			else
				((uint8_t*)&registers[reg - 4])[1] = xor_8(registers[reg - 4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));
			IP += ipOffset;
			break;
		}

		case 0x33:
		{ /* xor reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = xor_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0));
			IP += ipOffset;
			break;
		}

		case 0x34:
		{ /* xor al, imm8 */
			((uint8_t *)&AX)[0] = xor_8((uint8_t)AX, ram_get_8(CS, IP++));
			break;
		}

		case 0x35:
		{ /* xor ax, imm16 */
			AX = xor_16(AX, ram_get_16(CS, IP));
			IP += 2;
			break;
		}

		case 0x37:
		{ /* aaa */
			if ((AX & 0xf) > 9 || (FLAGS & AF)) {
				uint8_t al = AX & 0xff;
				al += 6;
				AX &= 0xff00;
				AX |= al;

				uint8_t ah = AX >> 8;
				ah++;
				AX &= 0x00ff;
				AX |= ah << 8;

				FLAGS |= AF;
				FLAGS |= CF;
			}
			else {
				FLAGS &= ~AF;
				FLAGS &= ~CF;
			}
			AX &= 0xff0f;
			break;
		}

		case 0x38:
		{ /* cmp rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				sub_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg] & 0xff);
			else
				sub_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8);
			IP += ipOffset;
			break;
		}

		case 0x39:
		{ /* cmp rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			sub_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]);
			IP += ipOffset;
			break;
		}

		case 0x3a:
		{ /* cmp reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				sub_8((uint8_t)registers[reg], get_with_mode_8(segment, mode, rm, ipOffset));
			else
				sub_8(registers[reg - 4] >> 8, get_with_mode_8(segment, mode, rm, ipOffset));
			IP += ipOffset;
			break;
		}

		case 0x3b:
		{ /* cmp reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			sub_16(registers[reg], get_with_mode(segment, mode, rm, ipOffset, 0));
			IP += ipOffset;
			break;
		}

		case 0x3c:
		{ /* cmp al, imm8 */
			uint8_t imm8 = ram_get_8(CS, IP++);
			sub_8((uint8_t)AX, imm8);
			break;
		}

		case 0x3d:
		{ /* cmp ax, imm16 */
			uint16_t imm16 = ram_get_16(CS, IP);
			IP += 2;
			sub_16(AX, imm16);
			break;
		}

		case 0x3f:
		{/* aas */
			if ((AX & 0xf) > 9 || (FLAGS & AF)) {
				uint8_t al = AX & 0xff;
				al -= 6;
				AX &= 0xff00;
				AX |= al;

				uint8_t ah = AX >> 8;
				ah--;
				AX &= 0x00ff;
				AX |= ah << 8;

				FLAGS |= AF;
				FLAGS |= CF;
			}
			else {
				FLAGS &= ~AF;
				FLAGS &= ~CF;
			}
			AX &= 0xff0f;
			break;
		}

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		{ /* inc */
			registers[op - 0x40] = inc_16(registers[op - 0x40]);
			break;
		}

		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
		{ /* dec */
			registers[op - 0x48] = dec_16(registers[op - 0x48]);
			break;
		}

		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x55:
		case 0x56:
		case 0x57:
		{ /* push */
			push(registers[op - 0x50]);
			break;
		}

		case 0x54:
		{ /* push sp */
			SP -= 2;
			ram_set_16(SS, SP, SP);
			break;
		}

		case 0x58:
		case 0x59:
		case 0x5a:
		case 0x5b:
		case 0x5c:
		case 0x5d:
		case 0x5e:
		case 0x5f:
		{ /* pop */
			registers[op - 0x58] = pop();
			break;
		}

		case 0x60:
		{ /* pusha */
			uint16_t tmp = SP;
			push(AX);
			push(CX);
			push(DX);
			push(BX);
			push(tmp);
			push(BP);
			push(SI);
			push(DI);
			break;
		}

		case 0x61:
		{ /* popa */
			DI = pop();
			SI = pop();
			BP = pop();
			SP += 2; // do not pop SP off stack
			BX = pop();
			DX = pop();
			CX = pop();
			AX = pop();
			break;
		}

		case 0x66:
		{ /* Operand Size (80386+) */
			cout << "Warning: Only emulating an 80186. 80386 registers (eax, etc.) do not exist!" << endl;
			break;
		}

		case 0x68:
		{ /* push imm16 */
			push(ram_get_16(CS, IP));
			IP += 2;
			break;
		}

		case 0x69:
		{ /* imul reg16, rm16, imm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t value = get_with_mode(segment, mode, rm, ipOffset, 0);

			uint16_t imm16 = ram_get_16(CS, IP + ipOffset);

			registers[reg] = imul_16(value, imm16);

			IP += ipOffset + 2;
			break;
		}

		case 0x6a:
		{ /* push imm8 */
			push(ram_get_8(CS, IP++));
			break;
		}

		case 0x6b:
		{ /* imul reg16, rm16, imm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t value = get_with_mode(segment, mode, rm, ipOffset, 0);

			uint8_t imm8 = sign_extend(ram_get_8(CS, IP + ipOffset));

			registers[reg] = imul_16(value, (int16_t)imm8);

			IP += ipOffset + 1;
			break;
		}

		case 0x70:
			if (FLAGS & OF)
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x71:
			if (!(FLAGS & OF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x72:
			if (FLAGS & CF)
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x73:
			if (!(FLAGS & CF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x74:
			if (FLAGS & ZF)
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x75:
			if (!(FLAGS & ZF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x76:
			if ((FLAGS & CF) || (FLAGS & ZF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x77:
			if (!(FLAGS & CF) && !(FLAGS & ZF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x78:
			if (FLAGS & SF)
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x79:
			if (!(FLAGS & SF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x7a:
			if ((FLAGS & PF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x7b:
			if (!(FLAGS & PF))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x7c:
			if (((FLAGS & SF) >> SF_) != ((FLAGS & OF) >> OF_))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x7d:
			if (((FLAGS & SF) >> SF_) == ((FLAGS & OF) >> OF_))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x7e:
			if ((FLAGS & ZF) || (((FLAGS & SF) >> SF_) != ((FLAGS & OF) >> OF_)))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		case 0x7f:
			if (!(FLAGS & ZF) && ((FLAGS & SF) >> SF_) == ((FLAGS & OF) >> OF_))
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;

		case 0x80:
		case 0x82:
		{ /* add/and/xor/or/adc/cmp rm8, imm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint8_t value = get_with_mode_8(segment, mode, rm, ipOffset);

			uint8_t imm8 = ram_get_8(CS, IP + ipOffset);

			if (!reg)
				set_with_mode_8(segment, mode, rm, add_8(value, imm8), ipOffset);
			else if (reg == 2)
				set_with_mode_8(segment, mode, rm, add_8(value, imm8 + (FLAGS & CF)), ipOffset);
			else if (reg == 7)
				sub_8(value, imm8);
			else if (reg == 4)
				set_with_mode_8(segment, mode, rm, and_8(value, imm8), ipOffset);
			else if (reg == 6)
				set_with_mode_8(segment, mode, rm, xor_8(value, imm8), ipOffset);
			else if (reg == 1)
				set_with_mode_8(segment, mode, rm, or_8(value, imm8), ipOffset);
			else if (reg == 5)
				set_with_mode_8(segment, mode, rm, sub_8(value, imm8), ipOffset);
			else {
				cout << "unimplemented op 0x80 format, reg=" << reg << endl;
			}

			IP += ipOffset + 1;
			break;
		}

		case 0x81:
		{ /* add/and/xor/or/adc/cmp rm16, imm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t value = get_with_mode(segment, mode, rm, ipOffset, 0);

			uint16_t imm16 = ram_get_16(CS, IP + ipOffset);

			if (!reg)
				set_with_mode(segment, mode, rm, add_16(value, imm16), ipOffset);
			else if (reg == 2)
				set_with_mode(segment, mode, rm, add_16(value, imm16 + (FLAGS & 1)), ipOffset);
			else if (reg == 7)
				sub_16(value, imm16);
			else if (reg == 4)
				set_with_mode(segment, mode, rm, and_16(value, imm16), ipOffset);
			else if (reg == 6)
				set_with_mode(segment, mode, rm, xor_16(value, imm16), ipOffset);
			else if (reg == 1)
				set_with_mode(segment, mode, rm, or_16(value, imm16), ipOffset);
			else if (reg == 5)
				set_with_mode(segment, mode, rm, sub_16(value, imm16), ipOffset);
			else {
				cout << "unimplemented op 0x81 format, reg=" << reg << endl;
			}
			IP += ipOffset + 2;
			break;
		}

		case 0x83:
		{ /* add/and/xor/or/adc/cmp/test rm16, imm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t value = get_with_mode(segment, mode, rm, ipOffset, 0);

			uint8_t imm8 = ram_get_8(CS, IP + ipOffset);

			if (!reg)
				set_with_mode(segment, mode, rm, add_16(value, imm8), ipOffset);
			else if (reg == 2)
				set_with_mode(segment, mode, rm, add_16(value, imm8 + (FLAGS & 1)), ipOffset);
			else if (reg == 7)
				sub_16(value, sign_extend(imm8));
			else if (reg == 4)
				set_with_mode(segment, mode, rm, and_16(value, imm8), ipOffset);
			else if (reg == 6)
				set_with_mode(segment, mode, rm, xor_16(value, imm8), ipOffset);
			else if (reg == 1)
				set_with_mode(segment, mode, rm, or_16(value, imm8), ipOffset);
			else if (reg == 5)
				set_with_mode(segment, mode, rm, sub_16(value, imm8), ipOffset);
			else {
				// reg == 3
				set_with_mode(segment, mode, rm, sbb_16(value, imm8), ipOffset);
			}
			IP += ipOffset + 1;
			break;
		}

		case 0x84:
		{ /* test rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				and_8(get_with_mode_8(segment, mode, rm, ipOffset), (uint8_t)registers[reg]);
			else
				and_8(get_with_mode_8(segment, mode, rm, ipOffset), registers[reg - 4] >> 8);

			IP += ipOffset;

			break;
		}

		case 0x85:
		{ /* test rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			and_16(get_with_mode(segment, mode, rm, ipOffset, 0), registers[reg]);

			IP += ipOffset;

			break;
		}

		case 0x86:
		{ /* xchg rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint8_t tmp;
			if (reg < 4) {
				tmp = (uint8_t)registers[reg];
				registers[reg] &= 0xff00;
				registers[reg] |= get_with_mode_8(segment, mode, rm, ipOffset);
			}
			else {
				tmp = (uint8_t)(registers[reg - 4] >> 8);
				registers[reg - 4] &= 0x00ff;
				registers[reg - 4] |= get_with_mode_8(segment, mode, rm, ipOffset) << 8;
			}

			set_with_mode_8(segment, mode, rm, tmp, ipOffset);

			IP += ipOffset;
			break;
		}

		case 0x87:
		{ /* xchg rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			uint16_t tmp = registers[reg];

			int16_t ipOffset = 0;
			registers[reg] = get_with_mode(segment, mode, rm, ipOffset, 0);

			set_with_mode(segment, mode, rm, tmp, ipOffset);

			IP += ipOffset;
			break;
		}

		case 0x88:
		{ /* mov rm8, reg8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4)
				set_with_mode_8(segment, mode, rm, registers[reg] & 0xff, ipOffset);
			else
				set_with_mode_8(segment, mode, rm, registers[reg - 4] >> 8, ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x89:
		{ /* mov rm16, reg16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, registers[reg], ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x8a:
		{ /* mov reg8, rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (reg < 4) {
				registers[reg] &= 0xff00;
				registers[reg] |= get_with_mode_8(segment, mode, rm, ipOffset);
			}
			else {
				registers[reg - 4] &= 0x00ff;
				registers[reg - 4] |= (uint16_t)get_with_mode_8(segment, mode, rm, ipOffset) << 8;
			}
			IP += ipOffset;
			break;
		}

		case 0x8b:
		{ /* mov reg16, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = get_with_mode(segment, mode, rm, ipOffset, 0);
			IP += ipOffset;
			break;
		}

		case 0x8c:
		{ /* mov rm16, seg */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, segment_registers[reg], ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x8d:
		{ /* lea reg16, mem16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			registers[reg] = get_with_mode(segment, mode, rm, ipOffset, 0, true);
			IP += ipOffset;
			break;
		}

		case 0x8e:
		{ /* mov seg, rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			segment_registers[reg] = get_with_mode(segment, mode, rm, ipOffset, 0);
			IP += ipOffset;
			break;
		}

		case 0x8f:
		{ /* pop rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, registers[reg], ipOffset);
			IP += ipOffset;
			break;
		}

		case 0x90:
		{ /* nop (xchg ax, ax) */
			//cout << "NOP (XCHG AX, AX)" << endl;
			if (CS == 0x9fC0 && IP < 256) {
				cout << "Interrupt " << (IP - 1) << " still has default IVT value!" << endl;
				return 2;
			}
			break;
		}

		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		{
			// xchg does not affect FLAGS
			uint16_t tmp = AX;
			AX = registers[op - 0x90];
			registers[op - 0x90] = tmp;
			break;
		}

		case 0x98:
		{ /* cbw */
			AX = sign_extend((uint8_t)AX);
			break;
		}

		case 0x99:
		{ /* cwd */
			if (AX & 0x8000)
				DX = 0xffff;
			else
				DX = 0;
			break;
		}

		case 0x9a:
		{ /* call ptr16:16 */
			uint16_t addr = ram_get_16(CS, IP);
			IP += 2;
			uint16_t newcs = ram_get_16(CS, IP);
			IP += 2;

			push(CS);
			push(IP);
			CS = newcs;
			IP = addr;

			break;
		}

		case 0x9c:
		{ /* pushf */
			push(FLAGS);
			break;
		}

		case 0x9d:
		{ /* popf */
			FLAGS = pop();
			FLAGS |= 0xf000;
			FLAGS &= ~0x2a;
			break;
		}

		case 0x9f:
		{ /* lahf */
			((uint8_t *)&AX)[1] = (uint8_t)FLAGS;
			break;
		}

		case 0xa0:
		{ /* mov al, [x] */
			uint16_t addr = ram_get_16(CS, IP);
			IP += 2;

			AX &= 0xff00;
			AX |= ram_get_8(segment, addr);
			break;
		}

		case 0xa1:
		{ /* mov ax, [x] */
			uint16_t addr = ram_get_16(CS, IP);
			IP += 2;

			AX = ram_get_16(segment, addr);
			break;
		}

		case 0xa2:
		{ /* mov [x], al */
			uint16_t off = ram_get_16(CS, IP);
			IP += 2;

			ram_set_8(segment, off, AX & 0xff);
			break;
		}

		case 0xa3:
		{ /* mov [x], ax */
			uint16_t off = ram_get_16(CS, IP);
			IP += 2;

			ram_set_16(segment, off, AX);
			break;
		}

		case 0xa8:
		{ /* test al, imm8 */
			and_8((uint8_t)AX, ram_get_8(CS, IP++));
			break;
		}

		case 0xa9:
		{ /* test ax, imm16 */
			and_16(AX, ram_get_16(CS, IP));
			IP += 16;
			break;
		}

		case 0x6c:
		case 0x6d:
		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa7:
		case 0xaa:
		case 0xab:
		case 0xac:
		case 0xad:
		case 0xae:
		case 0xaf:
		{
			int error = repeatableop();
			if (error) return 2;
			break;
		}

		case 0xb0:
		case 0xb1:
		case 0xb2:
		case 0xb3:
		{ /* mov reg8, imm8 */
			registers[op - 0xb0] &= 0xff00;
			registers[op - 0xb0] |= ram_get_8(CS, IP++);
			break;
		}

		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7:
		{ /* mov reg8, imm8 */
			registers[op - 0xb4] &= 0x00ff;
			registers[op - 0xb4] |= ((uint16_t)ram_get_8(CS, IP++)) << 8;
			break;
		}

		case 0xb8:
		case 0xb9:
		case 0xba:
		case 0xbb:
		case 0xbc:
		case 0xbd:
		case 0xbe:
		case 0xbf:
		{ /* mov reg16, imm16 */
			registers[op - 0xb8] = ram_get_16(CS, IP);
			IP += 2;
			break;
		}

		case 0xc0:
		{ /* ... rm8, imm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint8_t rm8 = get_with_mode_8(segment, mode, rm, ipOffset);

			uint8_t imm8 = ram_get_8(CS, IP + ipOffset);

			if (!reg) {
				/* rol rm8, imm8 */
				set_with_mode_8(segment, mode, rm, rol_8(rm8, imm8), ipOffset);
			}
			else if (reg == 4) {
				/* shl rm8, imm8 */
				set_with_mode_8(segment, mode, rm, shl_8(rm8, imm8), ipOffset);
			}
			else if (reg == 2) {
				/* rcl rm8, imm8 */
				set_with_mode_8(segment, mode, rm, rcl_8(rm8, imm8), ipOffset);
			}
			else if (reg == 3) {
				/* rcr rm8, imm8 */
				set_with_mode_8(segment, mode, rm, rcr_8(rm8, imm8), ipOffset);
			}
			else if (reg == 1) {
				/* ror rm8, imm8 */
				set_with_mode_8(segment, mode, rm, ror_8(rm8, imm8), ipOffset);
			}
			else if (reg == 5) {
				/* shr rm8, imm8 */
				set_with_mode_8(segment, mode, rm, shr_8(rm8, imm8), ipOffset);
			}
			else {
				cout << "unimplemented 0xc0 format, reg = " << reg << endl;
				return 2;
			}

			IP += ipOffset + 1;
			break;
		}

		case 0xc1:
		{ /* ... rm16, imm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t rm16 = get_with_mode(segment, mode, rm, ipOffset, 0);

			uint8_t imm8 = ram_get_8(CS, IP + ipOffset);

			if (!reg) {
				/* rol rm16, imm8 */
				set_with_mode(segment, mode, rm, rol_16(rm16, imm8), ipOffset);
			}
			else if (reg == 4) {
				/* shl rm16, imm8 */
				set_with_mode(segment, mode, rm, shl_16(rm16, imm8), ipOffset);
			}
			else if (reg == 2) {
				/* rcl rm16, imm8 */
				set_with_mode(segment, mode, rm, rcl_16(rm16, imm8), ipOffset);
			}
			else if (reg == 3) {
				/* rcr rm16, imm8 */
				set_with_mode(segment, mode, rm, rcr_16(rm16, imm8), ipOffset);
			}
			else if (reg == 1) {
				/* ror rm16, imm8 */
				set_with_mode(segment, mode, rm, ror_16(rm16, imm8), ipOffset);
			}
			else if (reg == 5) {
				/* shr rm16, imm8 */
				set_with_mode(segment, mode, rm, shr_16(rm16, imm8), ipOffset);
			}
			else {
				cout << "unimplemented 0xc1 format, reg = " << reg << endl;
				return 2;
			}

			IP += ipOffset + 1;
			break;
		}

		case 0xc2:
		{ /* ret imm16 */
			int16_t x = (int16_t)ram_get_16(CS, IP);

			IP = pop();
			SP += x;
			break;
		}

		case 0xc3:
		{ /* ret */
			IP = pop();
			break;
		}

		case 0xc5:
		{ /* lds reg16, mem16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t ramAddress = get_with_mode(segment, mode, rm, ipOffset, 0);

			uint16_t w0 = ram_get_16(segment, ramAddress);
			uint16_t w1 = ram_get_16(segment, ramAddress+2);

			registers[reg] = w0;
			DS = w1;

			IP += ipOffset;
			break;
		}

		case 0xc6:
		{ /* mov [rm], imm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode_8(segment, mode, rm, -1, ipOffset);
			IP += ipOffset + 1;
			break;
		}

		case 0xc7:
		{ /* mov [rm], imm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			set_with_mode(segment, mode, rm, -1, ipOffset);
			IP += ipOffset + 2;
			break;
		}

		case 0xca:
		{ /* retf imm16 */
			int16_t x = (int16_t)ram_get_16(CS, IP);
			IP += 2;

			IP = pop();
			CS = pop();
			SP += x;
			break;
		}

		case 0xcb:
		{ /* retf */
			IP = pop();
			CS = pop();
			break;
		}

		case 0xcc:
		{ /* int 3 */
			interrupt(3);
			break;
		}

		case 0xcd:
		{ /* int imm8 */
			uint8_t n = ram_get_8(CS, IP++);
			if (n == 0xcd) {
				cout << "Interrupt 0xcd!" << endl;
				return 2; // !! TODO remove this, for debugging purposes only
			}
			interrupt(n);
			break;
		}

		case 0xce:
		{ /* into */
			if ((FLAGS & OF))
				interrupt(4);
			break;
		}

		case 0xcf:
		{ /* iret */
			IP = pop();
			CS = pop();
			FLAGS = pop();
			FLAGS |= 0xf000;
			FLAGS &= ~0x2a;
			break;
		}

		case 0xd0:
		{ /* ... rm8, 1 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint8_t rm8 = get_with_mode_8(segment, mode, rm, ipOffset);

			if (!reg) {
				/* rol rm8, 1 */
				set_with_mode_8(segment, mode, rm, rol_8(rm8, 1), ipOffset);
			}
			else if (reg == 4) {
				/* shl rm8, 1 */
				set_with_mode_8(segment, mode, rm, shl_8(rm8, 1), ipOffset);
				break;
			}
			else if (reg == 2) {
				/* rcl rm8, 1 */
				set_with_mode_8(segment, mode, rm, rcl_8(rm8, 1), ipOffset);
				break;
			}
			else if (reg == 3) {
				/* rcr rm8, 1 */
				set_with_mode_8(segment, mode, rm, rcr_8(rm8, 1), ipOffset);
				break;
			}
			else if (reg == 7) {
				/* sar rm8, 1 */
				set_with_mode_8(segment, mode, rm, sar_8(rm8, 1), ipOffset);
			}
			else if (reg == 5) {
				/* shr rm8, 1 */
				set_with_mode_8(segment, mode, rm, shr_8(rm8, 1), ipOffset);
			}
			else if (reg == 1) {
				/* ror rm8, 1 */
				set_with_mode_8(segment, mode, rm, ror_8(rm8, 1), ipOffset);
			}
			else {
				cout << "unimplemented 0xd0 format, reg = " << reg << endl;
				return 2;
			}

			IP += ipOffset;

			break;
		}

		case 0xd1:
		{ /* ... rm8, 1 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t rm16 = get_with_mode(segment, mode, rm, ipOffset, 0);

			if (!reg) {
				/* rol rm16, 1 */
				set_with_mode(segment, mode, rm, rol_16(rm16, 1), ipOffset);
			}
			else if (reg == 4) {
				/* shl rm16, 1 */
				set_with_mode(segment, mode, rm, shl_16(rm16, 1), ipOffset);
			}
			else if (reg == 2) {
				/* rcl rm16, 1 */
				set_with_mode(segment, mode, rm, rcl_16(rm16, 1), ipOffset);
			}
			else if (reg == 3) {
				/* rcr rm16, 1 */
				set_with_mode(segment, mode, rm, rcr_16(rm16, 1), ipOffset);
			}
			else if (reg == 7) {
				/* sar rm16, 1 */
				set_with_mode(segment, mode, rm, sar_16(rm16, 1), ipOffset);
			}
			else if (reg == 5) {
				/* shr rm16, 1 */
				set_with_mode(segment, mode, rm, shr_16(rm16, 1), ipOffset);
			}
			else if (reg == 1) {
				/* ror rm16, 1 */
				set_with_mode(segment, mode, rm, ror_16(rm16, 1), ipOffset);
			}
			else {
				cout << "unimplemented 0xd1 format, reg = " << reg << endl;
				return 2;
			}

			IP += ipOffset;

			break;
		}

		case 0xd2:
		{ /* ... rm8, cl */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint8_t rm8 = get_with_mode_8(segment, mode, rm, ipOffset);

			if (!reg) {
				/* rol rm8, cl */
				set_with_mode_8(segment, mode, rm, rol_8(rm8, (uint8_t)CX), ipOffset);
			}
			else if (reg == 4) {
				/* shl rm8, cl */
				set_with_mode_8(segment, mode, rm, shl_8(rm8, (uint8_t)CX), ipOffset);
			}
			else if (reg == 2) {
				/* rcl rm8, cl */
				set_with_mode_8(segment, mode, rm, rcl_8(rm8, (uint8_t)CX), ipOffset);
			}
			else if (reg == 3) {
				/* rcr rm8, cl */
				set_with_mode_8(segment, mode, rm, rcr_8(rm8, (uint8_t)CX), ipOffset);
			}
			else if (reg == 7) {
				/* sar rm8, cl */
				set_with_mode_8(segment, mode, rm, sar_8(rm8, (uint8_t)CX), ipOffset);
			}
			else if (reg == 5) {
				/* shr rm8, cl */
				set_with_mode_8(segment, mode, rm, shr_8(rm8, (uint8_t)CX), ipOffset);
			}
			else if (reg == 1) {
				/* ror rm8, cl */
				set_with_mode_8(segment, mode, rm, ror_8(rm8, (uint8_t)CX), ipOffset);
			}
			else {
				cout << "unimplemented 0xd2 format, reg = " << reg << endl;
				return 2;
			}

			IP += ipOffset;

			break;
		}

		case 0xd3:
		{ /* ... rm16, cl */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t rm16 = get_with_mode(segment, mode, rm, ipOffset, 0);

			if (!reg) {
				/* rol rm16, cl */
				set_with_mode(segment, mode, rm, rol_16(rm16, (uint8_t)CX), ipOffset);
			}
			else if (reg == 4) {
				/* shl rm16, cl */
				set_with_mode(segment, mode, rm, shl_16(rm16, (uint8_t)CX), ipOffset);
			}
			else if (reg == 2) {
				/* rcl rm16, cl */
				set_with_mode(segment, mode, rm, rcl_16(rm16, (uint8_t)CX), ipOffset);
			}
			else if (reg == 3) {
				/* rcr rm16, cl */
				set_with_mode(segment, mode, rm, rcr_16(rm16, (uint8_t)CX), ipOffset);
			}
			else if (reg == 7) {
				/* sar rm16, cl */
				set_with_mode(segment, mode, rm, sar_16(rm16, (uint8_t)CX), ipOffset);
			}
			else if (reg == 5) {
				/* shr rm16, cl */
				set_with_mode(segment, mode, rm, shr_16(rm16, (uint8_t)CX), ipOffset);
			}
			else if (reg == 1) {
				/* ror rm16, cl */
				set_with_mode(segment, mode, rm, ror_16(rm16, (uint8_t)CX), ipOffset);
			}
			else {
				cout << "unimplemented 0xd3 format, reg = " << reg << endl;
				return 2;
			}

			IP += ipOffset;

			break;
		}

		case 0xd4:
		{ /* aam */
			IP++; // ignore base argument
			uint8_t ah = ((uint8_t)AX) / 10;
			uint8_t al = ((uint8_t)AX) % 10;
			AX = (ah << 8) | al;
			break;
		}

		case 0xd5:
		{ /* aad */
			IP++; // ignore base argument
			AX = (uint8_t)((AX >> 8) * 10 + (AX & 0xff));
			break;
		}

		case 0xe0:
		{ /* loopnz/loopne */
			int8_t rel8 = (int8_t)ram_get_8(CS, IP++);
			CX--;
			if (CX && !(FLAGS & ZF)) {
				IP += rel8;
			}
			break;
		}

		case 0xe1:
		{ /* loopz/loope */
			int8_t rel8 = (int8_t)ram_get_8(CS, IP++);
			CX--;
			if (CX && (FLAGS & ZF)) {
				IP += rel8;
			}
			break;
		}

		case 0xe2:
		{ /* loop */
			int8_t rel8 = (int8_t)ram_get_8(CS, IP++);
			CX--;
			if (CX) {
				IP += rel8;
			}
			break;
		}

		case 0xe3:
		{ /* JCXZ */
			if (!CX)
				IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		}

		case 0xe4:
		{ /* in al, imm8 */
			((uint8_t*)&AX)[0] = in_8(ram_get_8(CS, IP++));
			break;
		}

		case 0xe5:
		{ /* in ax, imm8 */
			AX = in_16(ram_get_16(CS, IP));
			IP += 2;
			break;
		}

		case 0xe6:
		{ /* out imm8, al */
			out_8(ram_get_8(CS, IP++), (uint8_t)AX);
			break;
		}

		case 0xe7:
		{ /* out imm8, ax */
			out_16(ram_get_16(CS, IP), AX);
			IP += 2;
			break;
		}

		case 0xe8:
		{ /* call rel16 */
			int16_t rel16 = (int16_t)ram_get_16(CS, IP);
			IP += 2;
			push(IP);
			IP += rel16;
			break;
		}

		case 0xe9:
		{ /* jmp rel16 */
			IP += (int16_t)ram_get_16(CS, IP);
			IP += 2;
			break;
		}

		case 0xeb:
		{ /* jmp rel8 */
			IP += (int8_t)ram_get_8(CS, IP);
			IP++;
			break;
		}

		case 0xec:
		{ /* in al, dx */
			((uint8_t*)&AX)[0] = in_8(DX);
			break;
		}

		case 0xed:
		{ /* in ax, dx */
			AX = in_16(DX);
			break;
		}

		case 0xea:
		{ /* jmp ptr16:16 */
			uint16_t cs_ = ram_get_16(CS, IP + 2);
			IP = ram_get_16(CS, IP);
			CS = cs_;
			break;
		}

		case 0xf4:
		{ /* hlt */
			if (!(FLAGS & IF)) {
				cout << "** HLT with IF = 0! **" << endl;
				return 2;
			}
			return 1;
		}

		case 0xf5:
		{ /* cmc */
			if (FLAGS & CF)
				FLAGS &= ~CF;
			else
				FLAGS |= CF;
			break;
		}

		case 0xf6:
		{
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint8_t rm8 = get_with_mode_8(segment, mode, rm, ipOffset);

			if (reg == 6) {
				/* div */
				if (!rm8) {
					cout << "Divide by 0! op 0xf6, op2 0x" << hex << (unsigned)op2 << dec << endl;
					interrupt(0);
				}
				else {
					uint8_t quotient = (uint8_t)(AX / rm8);
					uint8_t remainder = (uint8_t)(AX % rm8);
					AX = (remainder << 8) | quotient;
				}
			}
			else if (reg == 7) {
				/* idiv */
				if (!rm8) {
					cout << "Signed divide by 0! op 0xf6, op2 0x" << hex << (unsigned)op2 << dec << endl;
					interrupt(0);
				}
				else {
					int8_t quotient = (int8_t)((int16_t)AX / (int8_t)rm8);
					int8_t remainder = (int8_t)((int16_t)AX % (int8_t)rm8);
					AX = (remainder << 8) | quotient;
				}
			}
			else if (reg == 4) {
				/* mul */
				AX = (uint8_t)AX * rm8;
				if (AX & 0xff00)
					FLAGS |= OF | CF;
				else
					FLAGS &= ~(OF | CF);
			}
			else if (reg == 5) {
				/* mul */
				AX = imul_8((int8_t)AX, (int8_t)rm8);
			}
			else if (reg == 2) {
				/* not rm8 */
				set_with_mode_8(segment, mode, rm, ~rm8, ipOffset);
			}
			else if (reg == 3) {
				/* neg rm8 */
				set_with_mode_8(segment, mode, rm, -rm8, ipOffset);
			}
			else if (!reg) {
				/* test rm8, imm8 */
				and_8(rm8, ram_get_8(CS, IP + ipOffset));
				ipOffset += 1;
			}
			else {
				cout << "unrecognised op 0xf6 format. reg=" << reg << endl;
				return 2;
			}
			IP += ipOffset;
			break;
		}

		case 0xf7:
		{
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			uint16_t rm16 = get_with_mode(segment, mode, rm, ipOffset, 0);

			if (reg == 6) {
				/* div */
				if (!rm16) {
					cout << "Divide by 0! op 0xf7, op2 0x" << hex << (unsigned)op2 << dec << endl;
					interrupt(0);
				}
				else {
					uint32_t value = (DX << 16) | AX;
					AX = (uint16_t)(value / rm16);
					DX = (uint16_t)(value % rm16);
				}
			}
			else if (reg == 7) {
				/* idiv */
				if (!rm16) {
					cout << "Signed divide by 0! op 0xf7, op2 0x" << hex << (unsigned)op2 << dec << endl;
					interrupt(0);
				}
				else {
					int32_t value = (DX << 16) | AX;
					AX = (uint16_t)((int16_t)value / (int16_t)rm16);
					DX = (uint16_t)((int16_t)value % (int16_t)rm16);
				}
			}
			else if (reg == 4) {
				/* mul */
				uint32_t result = AX * rm16;
				AX = (uint16_t)result;
				DX = (uint16_t)(result >> 16);
				if (DX)
					FLAGS |= OF | CF;
				else
					FLAGS &= ~(OF | CF);
			}
			else if (reg == 5) {
				/* mul */
				int32_t result = imul_16((int16_t)AX, (int16_t)rm16);
				AX = (uint16_t)result;
				DX = (uint16_t)(result>>16);
			}
			else if (reg == 2) {
				/* not rm16 */
				set_with_mode(segment, mode, rm, ~rm16, ipOffset);
			}
			else if (reg == 3) {
				/* neg rm16 */
				set_with_mode(segment, mode, rm, -rm16, ipOffset);
			}
			else if (!reg) {
				/* test rm16, imm16 */
				and_16(rm16, ram_get_16(CS, IP + ipOffset));
				ipOffset += 2;
			}
			else {
				cout << "unrecognised op 0xf7 format. reg=" << reg << endl;
				return 2;
			}
			IP += ipOffset;
			break;
		}

		case 0xf8:
			FLAGS &= ~CF;
			break;

		case 0xf9:
			FLAGS |= CF;
			break;

		case 0xfa:
			FLAGS &= ~IF;
			break;

		case 0xfb:
			FLAGS |= IF;
			break;

		case 0xfc:
			FLAGS &= ~DF;
			break;

		case 0xfd:
			FLAGS |= DF;
			break;

		case 0xfe:
		{ /* inc/dec rm8 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;
			if (!reg)
				set_with_mode_8(segment, mode, rm, inc_8(get_with_mode_8(segment, mode, rm, ipOffset)), ipOffset);
			else if (reg == 1)
				set_with_mode_8(segment, mode, rm, dec_8(get_with_mode_8(segment, mode, rm, ipOffset)), ipOffset);
			IP += ipOffset;
			break;
		}

		case 0xff:
		{ /* call/push/inc/dec rm16 */
			uint8_t op2 = ram_get_8(CS, IP++);

			int rm = op2 & 7;
			int reg = (op2 >> 3) & 7;
			int mode = (op2 >> 6) & 3;

			int16_t ipOffset = 0;

			if (reg == 2) { /* call rm16 */
				uint16_t x = (uint16_t)get_with_mode(segment, mode, rm, ipOffset, 0);
				push(IP);
				IP = x;
				break;
			}
			else if (reg == 6) {
				push(get_with_mode(segment, mode, rm, ipOffset, 0));
			}
			else if (!reg) {
				set_with_mode(segment, mode, rm, inc_16(get_with_mode(segment, mode, rm, ipOffset, 0)), ipOffset);
			}
			else if (reg == 1) {
				set_with_mode(segment, mode, rm, dec_16(get_with_mode(segment, mode, rm, ipOffset, 0)), ipOffset);
			}
			else if (reg == 4) {
				IP = get_with_mode(segment, mode, rm, ipOffset, 0);
				break;
			}
			else if (reg == 5) {
				uint16_t seg;
				uint16_t x = get_with_mode(segment, mode, rm, ipOffset, &seg);
				CS = seg;
				IP = x;
				break;
			}
			else { /* call m16:16 */
				uint16_t cs;
				IP = get_with_mode(segment, mode, rm, ipOffset, &cs);
				CS = cs;
				break;
			}
			IP += ipOffset;
			break;
		}

		default:
			cout << "Unrecognised instruction: " << hex << (unsigned)op << " (" << dec << (unsigned)op << ")" << endl;
			return 2;
		}
		return 0;
	}

	void create()
	{
		ivt = (IVTEntry *)ram_8;
	}

	void dump()
	{
		cout << "*** CPU DUMP ***" << endl;
		cout << "AX: " << hex << AX << " (" << dec << AX << ")" << endl;
		cout << "BX: " << hex << BX << " (" << dec << BX << ")" << endl;
		cout << "CX: " << hex << CX << " (" << dec << CX << ")" << endl;
		cout << "DX: " << hex << DX << " (" << dec << DX << ")" << endl;
		cout << "SP: " << hex << SP << " (" << dec << SP << ")" << endl;
		cout << "BP: " << hex << BP << " (" << dec << BP << ")" << endl;
		cout << "SI: " << hex << SI << " (" << dec << SI << ")" << endl;
		cout << "DI: " << hex << DI << " (" << dec << DI << ")" << endl;
		cout << "CS: " << hex << CS << " (" << dec << CS << ")" << endl;
		cout << "DS: " << hex << DS << " (" << dec << DS << ")" << endl;
		cout << "ES: " << hex << ES << " (" << dec << ES << ")" << endl;
		cout << "SS: " << hex << SS << " (" << dec << SS << ")" << endl;
		cout << "FS: " << hex << FS << " (" << dec << FS << ")" << endl;
		cout << "GS: " << hex << GS << " (" << dec << GS << ")" << endl;
		cout << "FLAGS: " << hex << FLAGS << endl;
		cout << "IP: " << hex << IP << " (" << dec << IP << ")" << endl;
		cout << "********" << endl;
	}

};
