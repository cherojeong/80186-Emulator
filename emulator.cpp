#include "CPU.h"
#include "system.h"
#include "graphics.h"

#include <iostream>
#include <thread>

using namespace std;

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

	create_window();

	while (1) {
		int halted = CPU::step();

		if (halted == 2) {
			break;
		}

		int r = update_window();
		if (r)
			break;

		if (halted == 1) { // halted with interrupts enabled
			//this_thread::sleep_for(2ms);
		}
		else if (halted == 3) { // waiting for user input
			this_thread::sleep_for(15ms);
		}
	}

	destroy_window();

	CPU::dump();
	System::dump_ram("ram.bin");
	System::destroy();

}