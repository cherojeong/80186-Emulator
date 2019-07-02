#include "CPU.h"
#include "system.h"
#include "graphics.h"

#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

atomic<bool> stop = false;

void sdl_thread()
{
	create_window();
	while (!stop) {
		int r = update_window(true);
		if (r)
			break;

		this_thread::sleep_for(chrono::milliseconds(32));
	}
	stop = true;
	destroy_window();
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (System::create()) {
		cout << "Error creating virtual PC!" << endl;
#ifdef _WIN32
		getc(stdin);
#endif
		return 1;
	}


	thread t(sdl_thread);

	while (!stop) {
		for (int i = 0; i < 10000; i++) {
			int halted = CPU::step();

			if (halted == 2 || halted == 4) {
				stop = true;
				break;
			}

			if (halted == 1) { // halted with interrupts enabled
				this_thread::sleep_for(2ms);
			}
			else if (halted == 3) { // waiting for user input
				this_thread::sleep_for(30ms);

				if (stop) {
					break;
				}
			}
		}
	}

	t.join();


	CPU::dump();
	System::dump_ram("ram.bin");
	System::destroy();

}