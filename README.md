# MR_J_System





## Description
This is a Mr. J. System a system capable of infallibly distorting reality, resisting any type of failure and automatically recovering from any setback. It consists of a central server, Gotham, and several clients, Fleck, who will make distortion requests. The central server will manage these requests through specialized processes (workers): Enigma, which will be dedicated to text distortion, and Harley, in charge of manipulating audiovisual media (image and audio). The system will work on a Linux-based infrastructure, using a TCP/IP network, ensuring a fast and stable connection between the different components.
 

## Installation 🚀
To compile the project you need gcc and make installed on your computer.
To compile issue make command on the root directory of the project.
```bash
make
```
## Usage 💻
The first one that needs to be running is Gotham server, then Fleck client and finally the Workers.

### Server (Gotham)
To run the server you need to execute the following command on the root directory of the project:
```bash
Gotham Gotham.dat
```

### Client (Fleck)
To run the client you need to execute the following command on the root directory of the project:
```bash
Fleck Fleck.dat
```

### Workers (Enigma/Harley)
To run the workers you need to execute the following command on the root directory of the project:
```bash
Harley Harley.dat
```
```bash
Enigma Enigma.dat
```

## Valgrind
If you want to get a stricter compilation you can use " valgrind " in front of the previous commands.

Example:
```bash
valgrind Enigma Enigma.dat
```
```bash
valgrind Harley Harley.dat
```
```bash
valgrind Fleck Fleck.dat
```
```bash
valgrind Gotham Gotham.dat
```
