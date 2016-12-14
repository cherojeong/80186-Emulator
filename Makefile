CC=g++
CFLAGS=-c -std=c++14 -Wall

all: emulator.o 80186.o cmos.o PC.o graphics.o
	$(CC) *.o -o emx86_16 -lSDL2
    
emulator.o: emulator.cpp
	$(CC) $(CFLAGS) emulator.cpp -o emulator.o
    
80186.o: 80186.cpp
	$(CC) $(CFLAGS) 80186.cpp -o 80186.o
    
cmos.o: cmos.cpp
	$(CC) $(CFLAGS) cmos.cpp -o cmos.o
    
PC.o: PC.cpp
	$(CC) $(CFLAGS) PC.cpp -o PC.o
    
graphics.o: graphics.cpp
	$(CC) $(CFLAGS) graphics.cpp -o graphics.o

clean:
	rm *.o


