# Makefile to build BANK and ATM programs

CC = gcc
CFLAGS = -Wall -pthread

all: BANK ATM

BANK: BANK.c
	$(CC) $(CFLAGS) -o BANK BANK.c

ATM: ATM.c
	$(CC) $(CFLAGS) -o ATM ATM.c

clean:
	rm -f BANK ATM
