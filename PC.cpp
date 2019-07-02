#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <ctime>
#include <chrono>
#include "CPU.h"
#include "devices.h"
#include <SDL2/SDL.h>

using namespace std;

extern int keyPress;

namespace System {

	uint8_t * ram_8;
	uint16_t * ram_16;


	// Floppy disk is 5.25-inch double density 160KiB
	// 512 bps, 8 sectors per track, 40 tracks per side, 1 side

	// or

	// Floppy disk is 3.5-inch high density 1440KiB
	// 512 bps, 18 sectors per track, 80 tracks per side, 2 side

	size_t floppyFileSize;
	/* floppy disk is loaded into ram */
	char * floppyDisk;

#ifdef _WIN32
#pragma pack(1)
#endif

	typedef struct BDA
	{
		uint16_t com[4];
		uint16_t lpt[3];
		uint16_t ebda;
		uint16_t hardware;
		uint16_t kbdFlags;
		uint8_t kbdBuffer[32];
		uint8_t displayMode;
		uint16_t columns;
		uint16_t videoIoPort;
		uint16_t ticks;
		uint8_t diskDrives;
		uint16_t kbdBufferStart;
		uint16_t kbdBufferEnd;
		uint8_t lastKbdLEDState;
	}
#ifndef _WIN32
	__attribute__((packed))
#endif
	BDA;

	int create()
	{
		ram_8 = new uint8_t[1024*1024];
		ram_16 = (uint16_t *)ram_8;

		memset(ram_8, 0x90, 0xa0000);
		memset(&ram_8[0xa0000], 0, 1024 * 1024 - 0xa0000);

		/* Set up false interrupts */
		//for(int i = 0; i < 256; i++)
		//	ram_8[0x9fC00 + i] = 0x90; /* nop */

		for (uint16_t i = 0; i < 256; i++) {
			ram_16[i * 2] = i; // ip
			ram_16[i * 2 + 1] = 0x9fC0; // cs
		}

		BDA * bda = (BDA *)&ram_8[0x400];
		memset(bda, 0, sizeof(BDA));
		bda->ebda = 0x9fC00 >> 4;
		bda->hardware = 1 | (2 << 4);

		/*ifstream bios = ifstream("C:/Program Files (x86)/Bochs-2.6.8/BIOS-bochs-latest", ios::ate | ios::binary);
		if (!bios.is_open()) {
			cout << "Error opening bios rom" << endl;
			return 1;
		}

		long biosFileSize = (size_t)bios.tellg();

		bios.seekg(0);
		bios.read((char *)&ram_8[0xe0000], biosFileSize);*/

		ifstream floppyFile = ifstream("D:/80186/mikeos.flp", ios::ate | ios::binary);

		if (!floppyFile.is_open()) {
			cout << "Error opening floppy" << endl;
			return 1;
		}

		floppyFileSize = (size_t)floppyFile.tellg();

		if (floppyFileSize != 160 * 1024 && floppyFileSize != 180 * 1024 && floppyFileSize != 1440 * 1024
			&& floppyFileSize != 360 * 1024) {
			cerr << "Unrecognised floopy disk size: " << (floppyFileSize / 1024) << "KiB" << endl;
			floppyFile.close();
			return 1;
		}

		floppyDisk = new char[floppyFileSize];

		floppyFile.seekg(0);
		floppyFile.read(floppyDisk, floppyFileSize);
		memcpy(&ram_8[0x7c00], floppyDisk, floppyFileSize > 512 ? 512 : floppyFileSize);

		floppyFile.close();

		CPU::create();
		Devices::create_cmos();

		return 0;
	}

	void dump_ram(const char * filename)
	{
		ofstream file(filename, ios::binary);
		file.write((const char *)ram_8, 1024 * 1024);
		file.close();
	}

	void destroy()
	{
		delete[] ram_8;
		delete[] floppyDisk;
	}

	static uint16_t x = 0;
	static uint16_t y = 0;
	static uint8_t attribute = 0x0f;

	static inline void scroll()
	{
		memcpy(&ram_8[0xb8000], &ram_8[0xb8000 + 80 * 2], 80*24*2);
		uint16_t blank = attribute << 8;
		uint16_t * vidmem = (uint16_t*)&ram_8[0xb8000 + 24 * 80 * 2];
		for (unsigned int i = 0; i < 80; i++)
			vidmem[i] = blank;
	}

	void text_mode_write_character(char c, char attrib=attribute)
	{
		if (x >= 80) {
			x = 0;
			y++;
		}
		if (y >= 25) {
			x = 0;
			y = 24;
			scroll();
		}
		attribute = attrib;
		ram_8[0xb8000 + y * 80 * 2 + x * 2] = c;
		ram_8[0xb8000 + y * 80 * 2 + x * 2 + 1] = attrib;
		x++;
	}

	uint16_t do_keypress()
	{
		uint8_t ah = 0, al;
		if (keyPress == SDLK_UP) {
			ah = 0x48;
		}
		else if (keyPress == SDLK_DOWN) {
			ah = 0x50;
		}
		if (keyPress > 255)
			al = 0;
		else
			al = (uint8_t)keyPress;
		keyPress = 0;
		return (ah << 8) | al;
	}

	uint8_t lastDiskOperationAH = 0;
	
	int bios_interrupt(uint8_t n, uint16_t * registers, uint16_t * segment_registers, uint16_t * FLAGS)
	{

		//if(n != 0x16)
		//	cout << "Interrupt 0x" << hex << (unsigned)n << ", AX = 0x" << registers[0] << dec << endl;

#define AX registers[0]
#define CX registers[1]
#define DX registers[2]
#define BX registers[3]
#define SP registers[4]
#define BP registers[5]
#define SI registers[6]
#define DI registers[7]

#define ES segment_registers[0]
#define CS segment_registers[1]
#define SS segment_registers[2]
#define DS segment_registers[3]
#define FS segment_registers[4]
#define GS segment_registers[5]

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

		if (n == 0x10) {
			if ((AX & 0xff00) == 0x0e00) {
				if ((char)AX == 0xa) {
					y++;
					if (y >= 25) {
						scroll();
						x = 0;
						y = 24;
					}
				}
				else if ((char)AX == 0xd)
					x = 0;
				else if ((char)AX == 8) {
					if (x || y) {
						if (!x) {
							y--;
							x = 79;
						}
						else {
							x--;
						}
						ram_8[0xb8000 + y * 80 * 2 + x * 2] = 0;
					}
				}
				else {
					text_mode_write_character((char)AX);
				}
			}
			else if ((AX & 0xff00) == 0x0900) {
				uint16_t oldx = x;
				uint16_t oldy = y;
				for (unsigned int i = 0; i < CX && i < 80 * 25; i++) {
					text_mode_write_character((char)AX, (char)BX);
				}
				x = oldx;
				y = oldy;
			}
			else if ((AX & 0xff00) == 0x0200) {
				x = (uint8_t)DX;
				y = (uint8_t)(DX >> 8);
				if (y == 25) {
					x = 0;
					y = 24;
					scroll();
				}
			}
			else if ((AX & 0xff00) == 0x0100) {
			}
			else if ((AX & 0xff00) == 0x0300) {
				/* Get cursor position and size */
				DX = (y << 8) | x;
				CX = 15 | (14<<8);
			}
			else if ((AX & 0xff00) == 0x0500) {
				/* Set active display page */
				if ((char)AX) {
					cout << "Support for multiple text pages is unimplemented!" << endl;
				}
				/*uint16_t * vidmem = (uint16_t*)&ram_8[0xb8000];
				for(unsigned int i = 0; i < 80*25; i++)
					vidmem[i] = attribute;*/
			}
			else if ((AX & 0xff00) == 0x0600) {
				/* Scroll up window */
				uint8_t lines = (uint8_t)AX;
				uint8_t blank = (uint8_t)(BX >> 8);
				uint8_t tlX = (uint8_t)CX;
				uint8_t tlY = CX >> 8;
				uint8_t brX = (uint8_t)DX;
				uint8_t brY = DX >> 8;
				cout << "scroll #" << (unsigned)lines << endl;
				cout << "tl " << (unsigned)tlX << "," << (unsigned)tlY << ". br " << (unsigned)brX << "," << (unsigned)brY << endl;
				if(tlX <= brX && tlY <= brY && brX < 80 && brY < 25) {
					unsigned int width = brX - tlX + 1;
					unsigned int height = brY - tlY + 1;
					if(!lines || height == 1) {
						if(width == 80) {
							uint16_t * vidmem = (uint16_t*)&ram_8[0xb8000 + tlY*80*2];
							for(unsigned int i = 0; i < 80*height; i++)
								vidmem[i] = blank;
						} else {
							uint16_t * vidmem = (uint16_t*)&ram_8[0xb8000 + tlY*80*2 + tlX*2];
							for(unsigned int j = 0; j < height; j++) {
								for(unsigned int i = 0; i < width; i++) {
									vidmem[i] = blank;
								}
								vidmem += 80*2;
							}
						}
					}  else {
						uint16_t * vidmemSrc = (uint16_t*)&ram_8[0xb8000 + brY*80*2 + tlX*2];
						uint16_t * vidmemDst = (uint16_t*)&ram_8[0xb8000 + (brY-1)*80*2 + tlX*2];
						for(unsigned int i = 0; i < height-1; i++) {
							memcpy(&vidmemDst[i], &vidmemSrc[i], width*2);
							vidmemSrc -= 80*2;
							vidmemDst -= 80*2;
						}
						vidmemDst = (uint16_t*)&ram_8[0xb8000 + (brY)*80*2 + tlX*2];
						for(unsigned int i = 0; i < width; i++) {
							vidmemDst[i] = blank;
						}
					}
				}
			}
			else if ((AX & 0xff00) == 0x0800) {
				AX = ram_8[0xb8000 + (y*80 + x) * 2] | (ram_8[0xb8000 + (y * 80 + x) * 2 + 1] << 8);
			}
			else if (AX == 0x1003) {
				// BL = 00h background intensity enabled
				// BL =	01h blink enabled
			}
			else
				cout << "int 0x10, ax = " << hex << AX << dec << endl;
		}
		else if (n == 0x19) {
			return 2;
		}
		else if (n == 0x13) {
			uint8_t AH = (uint8_t)(registers[0] >> 8);
			cout << "AH is 0x" << hex << (unsigned)AH << dec << endl;

			if (!AH) {
				/* reset disk system */
				AX = 0;
				*FLAGS &= ~CF;
			}
			else if (AH == 1) {
				/* get last status */
				AX &= 0x00ff;
				AX |= lastDiskOperationAH << 8;
				if (lastDiskOperationAH)
					*FLAGS |= CF;
				else
					*FLAGS &= ~CF;
			}
			else if (AH == 2) {
				/* read */
				int bytes = ((uint8_t)AX) * 512;
				uint8_t cylinder = (uint8_t)(CX >> 8);
				uint8_t sector = CX & 0x3f; // 2
				uint8_t head = (uint8_t)(DX >> 8);
				unsigned int offset = 0;
				if (floppyFileSize == 160 * 1024) {
					offset = 512 * (cylinder * 8 + (sector - 1));
				}
				else if (floppyFileSize == 180 * 1024) {
					offset = 512 * (cylinder * 9 + (sector - 1));
				}
				else if (floppyFileSize == 1440 * 1024) {
					offset = 512 * (18 * (cylinder*2 + head) + (sector - 1));
				}
				else if (floppyFileSize == 360 * 1024) {
					offset = 512 * ((cylinder*2 + head) * 9 + (sector - 1));
				}
				int ramAddress = ES * 16 + BX;

				if (!bytes || (ramAddress + bytes) > 1048576 || (offset + bytes) > floppyFileSize || (floppyFileSize == 163840 && head) || (floppyFileSize == 1474560 && head > 1)) {
					cout << "(INVALID) READING " << bytes << " BYTES FROM FLOPPY AT OFFSET 0x" << hex << offset << " TO RAM ADDRESS 0x" << ramAddress << dec << endl;
					*FLAGS |= CF;
					AX = 0x0200;
					CPU::dump();
					dump_ram("ram.bin");
					lastDiskOperationAH = 2;
				}
				else {
					cout << "READING " << bytes << " BYTES FROM FLOPPY AT OFFSET 0x" << hex << offset << " TO RAM ADDRESS 0x" << ramAddress << dec << endl;
					
					memcpy((char *)&ram_8[ramAddress], &floppyDisk[offset], bytes);

					*FLAGS &= ~CF;
					AX &= 0x00ff;
					lastDiskOperationAH = 0;
				}				

			}
			else if (AH == 3) {
				/* write */
				int bytes = (uint8_t)registers[0] * 512;
				uint8_t cylinder = (uint8_t)(CX >> 8);
				uint8_t sector = CX & 0x3f;
				uint8_t head = (uint8_t)(DX >> 8);
				//uint8_t drive = (uint8_t)DX;
				int offset = 0;
				if (floppyFileSize == 160 * 1024) {
					offset = 512 * (cylinder * 8 + (sector - 1));
				}
				else if (floppyFileSize == 1440 * 1024) {
					offset = 512 * (18 * (cylinder * 2 + head) + (sector - 1));
				}
				int ramAddress = ES * 16 + BX;

				if (!bytes || (ramAddress + bytes) > 1048576 || (floppyFileSize == 163840 && head) || (floppyFileSize == 1474560 && head > 1)) {
					*FLAGS |= CF;
					AX = 0x0200;
					lastDiskOperationAH = 2;
				}
				else {
					cout << "WRITING " << bytes << " BYTES TO FLOPPY AT OFFSET 0x" << hex << offset << " FROM RAM ADDRESS 0x" << ramAddress << dec << endl;
					
					memcpy(&floppyDisk[offset], (char *)&ram_8[ramAddress], bytes);

					*FLAGS &= ~CF;
					AX &= 0x00ff;
					lastDiskOperationAH = 0;
				}
			}
			else if (AH == 8) {
				/* Drive parameters */
				cout << "Drive params implementation may not be correct!" << endl;
				if (DX & 0xff) {
					*FLAGS |= CF;
					AX = 0x0700;
				}
				else {
					AX = 0;
					if (floppyFileSize == 1440 * 1024) {
						BX = 4;
						CX = 18 | (160 << 8);
						DX = (1 << 8) | 1;
					}
					else if (floppyFileSize == 160 * 1024) {
						BX = 0;
						CX = 8 | (40 << 8);
						DX = 1;
					}
					else if (floppyFileSize == 180 * 1024) {
						BX = 0;
						CX = 9 | (40 << 8);
						DX = 1;
					}
					*FLAGS &= ~CF;
				}
			}
			else {
				cout << "int 13h, ax=" << hex << AX << dec << endl;
			}
		}
		else if (n == 0x14) {
			if (!(AX & 0xff00)) {
				AX |= 0xffff;
				*FLAGS |= CF;
			}
			else if ((AX & 0xff00) == 0x0300) {
				AX = 0;
				*FLAGS |= CF;
			}
			else {
				cout << "int 0x14 (serial (?) port) AX=0x" << hex << AX << dec << endl;
				CPU::dump();
			}
			*FLAGS |= CF;
		}
		else if (n == 0x17) {
			if ((AX & 0xff00) == 0x0100) {
				AX |= 0xffff;
				*FLAGS |= CF;
			}
			else {
				cout << "int 0x17 (printer)" << endl;
				CPU::dump();
			}
			*FLAGS |= CF;
		}
		else if (n == 0x11) { // BIOS Equipment List
			AX = 1 | (1 << 2) | (2 << 4);
			//AX = 0x4227; // value returned by bochs
		}
		else if (n == 0x12) { // Get RAM size
			AX = 256;
		}
		else if (n == 0x16) { // Keyboard
			if (!(AX & 0xff00) || (AX & 0xff00) == 0x1000) {
				if (keyPress) {
					AX = do_keypress();
					return 0;
				}
				return 1;
			} 
			else if ((AX & 0xff00) == 0x0100 || (AX & 0xff00) == 0x1100) {
				if (keyPress) {
					*FLAGS |= ZF;
					AX = do_keypress();
				}
				else {
					*FLAGS &= ~ZF;
					AX = 0;
				}
			}
		}
		else if (n == 0x1a) {
			if (!(AX & 0xff00)) {
				/* Get clock ticks since midnight */

				static int prevDay = -1;


				auto now = chrono::system_clock::now();

				/* Get midnight */
				time_t tnow = chrono::system_clock::to_time_t(now);
				tm *date = std::localtime(&tnow);
				date->tm_hour = 0;
				date->tm_min = 0;
				date->tm_sec = 0;
				auto midnight = chrono::system_clock::from_time_t(mktime(date));

				uint32_t ticks = (uint32_t)(chrono::duration_cast<chrono::milliseconds>(now - midnight).count() / 55);
				DX = (uint16_t)ticks;
				CX = ticks >> 16;

				static auto lastTime = chrono::system_clock::now();

				if (prevDay == -1 || prevDay == date->tm_yday) {
					AX &= 0xff00;
				}
				else {
					AX |= 1;
				}
				prevDay = date->tm_yday;

			}
		}
		else {
			cout << "interrupt " << (unsigned)n << " (0x" << hex << (unsigned)n << ")" << endl << dec;
		}
		return 0;
	}

	typedef uint8_t (*f_io_read_8) (uint16_t);
	typedef uint16_t (*f_io_read_16) (uint16_t);
	typedef void (*f_io_write_8) (uint16_t, uint8_t);
	typedef void (*f_io_write_16) (uint16_t, uint16_t);

	typedef struct IoPort
	{
		uint16_t port;
		f_io_read_8 io_read_8;
		f_io_read_16 io_read_16;
		f_io_write_8 io_write_8;
		f_io_write_16 io_write_16;
	} IoPort;

	std::vector<IoPort> ioports;

	IoPort get_port(uint16_t i)
	{
		for (const auto& p : ioports) {
			if (p.port == i) return p;
		}
		return IoPort{};
	}

	uint8_t in_8(uint16_t port)
	{
		cout << "Input byte from i/o port " << hex << port << dec << endl;
		IoPort io = get_port(port);
		if (io.io_read_8) {
			return io.io_read_8(port);
		}
		return 0;
	}

	uint16_t in_16(uint16_t port)
	{
		cout << "Input word from i/o port " << hex << port << dec << endl;
		IoPort io = get_port(port);
		if (io.io_read_16) {
			return io.io_read_16(port);
		}
		return 0;
	}

	void out_8(uint16_t port, uint8_t value)
	{
		cout << "Output byte 0x" << hex << (unsigned)value << " to i/o port " << port << dec << endl;
		IoPort io = get_port(port);
		if (io.io_write_8) {
			io.io_write_8(port, value);
		}
	}

	void out_16(uint16_t port, uint16_t value)
	{
		cout << "Output word 0x" << hex << value << " to i/o port " << port << dec << endl;
		IoPort io = get_port(port);
		if (io.io_write_16) {
			io.io_write_16(port, value);
		}
	}

	void register_io_port(uint16_t port, void * a, void * b, void * c, void * d)
	{
		IoPort io;
		io.port = port;
		io.io_read_8 = (f_io_read_8)a;
		io.io_read_16 = (f_io_read_16)b;
		io.io_write_8 = (f_io_write_8)c;
		io.io_write_16 = (f_io_write_16)d;

		ioports.push_back(io);
	}

};
