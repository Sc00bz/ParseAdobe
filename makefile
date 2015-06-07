CC=g++
FLAGS64=-Wall -m64 -O2
FLAGS32=-Wall -m32 -O2

all: parseadobe64 parseadobe32
	

# 64 bit
parseadobe64: parseadobe.cpp
	$(CC) $(FLAGS64) -o parseadobe64 parseadobe.cpp

# 32 bit
parseadobe32: parseadobe.cpp
	$(CC) $(FLAGS32) -o parseadobe32 parseadobe.cpp

clean:
	-rm *.o parseadobe64 parseadobe32
