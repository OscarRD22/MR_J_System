CC = gcc
CFLAGS = -Wall -Wextra -g 
FFLAGS = -lpthread 

all: Fleck Gotham Harley Enigma

# Specify dependencies for object files
io_utils.o: utils/io_utils.c
	$(CC) $(CFLAGS) -c utils/io_utils.c -o io_utils.o 

utils_fleck.o: utils/utils_fleck.c
	$(CC) $(CFLAGS) -c utils/utils_fleck.c -o utils_fleck.o

# Specify dependencies for executables
Fleck: fleck.c io_utils.o utils_fleck.o
	$(CC) $(CFLAGS) -o Fleck fleck.c io_utils.o utils_fleck.o $(FFLAGS)

Gotham: gotham.c
	$(CC) $(CFLAGS) -o Gotham gotham.c $(FFLAGS)

Harley: harley.c
	$(CC) $(CFLAGS) -o Harley harley.c $(FFLAGS)

Enigma: enigma.c
	$(CC) $(CFLAGS) -o Enigma enigma.c $(FFLAGS)

# Clean commands
clean:
	rm -vf *.o

clean_all: clean
	rm -rvf Fleck Gotham Harley Enigma  *.txt *.dSYM
