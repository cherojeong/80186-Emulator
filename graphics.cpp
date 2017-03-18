#include <SDL2/SDL.h>
#include <stdint.h>
#include <cstring>
#include <chrono>
#include <iostream>
#include <fstream>
#include "graphics.h"
#include "system.h"

using namespace std;

SDL_Window * window;
SDL_Surface * surface;
uint32_t * pixels;
chrono::high_resolution_clock::time_point lastFrameTime;

uint8_t vga_font[256 * 16];

void create_window()
{
	window = SDL_CreateWindow("x86_16", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 400, 0);
	surface = SDL_GetWindowSurface(window);
	pixels = (uint32_t *)surface->pixels;
	lastFrameTime = chrono::high_resolution_clock::now();
	update_window(true);

	ifstream f = ifstream("font0.bin", ios::binary);
	if (!f.is_open())
		throw runtime_error("VGA Font file not found");

	f.read((char*)vga_font, 256*16);
	f.close();
}

void destroy_window()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int keyPress = 0;

int update_window(bool forceUpdate)
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT) {
			return 1;
		}
#ifdef _WIN32
		if (e.type == SDL_KEYDOWN) {
			keyPress = e.key.keysym.sym;
		}
#else
		/* SDL2 keyboard input seems to behave differently on GNU/Linux */
		if (e.type == SDL_KEYDOWN && e.key.state == SDL_PRESSED && !e.key.repeat) {
			keyPress = e.key.keysym.sym;
	}
#endif
	}

	auto now = chrono::high_resolution_clock::now();
	double deltaTime = chrono::duration_cast<chrono::microseconds>(now - lastFrameTime).count() / 1000000.0f;

	if ((deltaTime > (1.0 / 30.0)) || forceUpdate) {
		uint8_t * c = &System::ram_8[0xb8000];
		for (int y = 0; y < 25; y++) {
			for (int x = 0; x < 80; x++) {
				uint8_t character = c[0];
				uint8_t attribute = c[1];
				c += 2;

				static uint32_t colours[] = {
					0xff000000,
					0xff0000aa,
					0xff00aa00,
					0xff00aaaa,
					0xffaa0000,
					0xffaa00aa,
					0xffaa5500,
					0xffaaaaaa,

					0xff555555,
					0xff5555ff,
					0xff55ff55,
					0xff55ffff,
					0xffff5555,
					0xffff55ff,
					0xffffff00,
					0xffffffff
				};

				uint32_t fg = colours[attribute & 0xf];
				uint32_t bg = colours[attribute >> 4];

				int fontOffset = character * 16;
				int framebufferOffset = (y * 80 * 9 * 16) + (x*9);
				for (int i = 0; i < 16; i++) {
					uint8_t fontByte = vga_font[fontOffset++];
					int off = framebufferOffset;
					for (int j = 0; j < 8; j++) {
						if (fontByte & (1 << (8-j))) {
							pixels[off] = fg;
						}
						else {
							pixels[off] = bg;
						}
						off++;
					}
					pixels[off] = bg;
					framebufferOffset += 80 * 9;
				}
			}
		}

		SDL_UpdateWindowSurface(window);
		lastFrameTime = now;
	}

	return 0;

}
