CC = g++

SRC = src
BIN = bin

all: Alpha_Blending clean

Alpha_Blending: main.o Alpha_Blending.o 
	$(CC) $(BIN)\main.o $(BIN)\Alpha_Blending.o -o $(BIN)\Alpha_Blending.exe

main.o:
	$(CC) -c $(SRC)\main.cpp -o $(BIN)\main.o

Alpha_Blending.o:
	$(CC) -c -msse4.2 $(OPT) $(MODE) $(SRC)\Alpha_Blending.cpp -o $(BIN)\Alpha_Blending.o

run:
	$(BIN)\Alpha_Blending.exe $(FR) $(BK)

clean:
	del $(BIN)\*.o
