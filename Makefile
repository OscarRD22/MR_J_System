CC = gcc
CFLAGS = -Wall -Wextra -g 
FFLAGS = -lpthread 

all: Fleck Gotham Harley Enigma

# Specify dependencies for object files
io_utils.o: utils/io_utils.c
	$(CC) $(CFLAGS) -c utils/io_utils.c -o io_utils.o 

utils_connect.o : utils/utils_connect.c
	$(CC) $(CFLAGS) -c utils/utils_connect.c -o utils_connect.o

utils_fleck.o: utils/utils_fleck.c utils_connect.o
	$(CC) $(CFLAGS) -c utils/utils_fleck.c -o utils_fleck.o

# Specify dependencies for executables
Fleck: fleck.c io_utils.o utils_fleck.o utils_connect.o
	$(CC) $(CFLAGS) -o Fleck fleck.c io_utils.o utils_fleck.o utils_connect.o $(FFLAGS)

Gotham: gotham.c io_utils.o utils_connect.o io_utils.o
	$(CC) $(CFLAGS) -o Gotham gotham.c utils_connect.o io_utils.o $(FFLAGS)

Harley: harley.c io_utils.o  utils_connect.o io_utils.o
	$(CC) $(CFLAGS) -o Harley harley.c utils_connect.o io_utils.o $(FFLAGS)

Enigma: enigma.c io_utils.o utils_connect.o io_utils.o
	$(CC) $(CFLAGS) -o Enigma enigma.c utils_connect.o io_utils.o $(FFLAGS)

# Clean commands
clean:
	rm -vf *.o

clean_all: clean
	rm -rvf Fleck Gotham Harley Enigma  *.txt *.dSYM