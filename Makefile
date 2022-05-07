CC = g++

SRC = src/
BIN = bin

all: Alpha_Blending

Alpha_Blending: main.o Alpha_Blending.o
	$(CC) $(BIN)main.o $(BIN)Alpha_Blending.o -o $(BIN)Alpha_Blending.exe
	del $(BIN)main.o $(BIN)Alpha_Blending.o

main.o:
	$(CC) -c $(SRC)main.cpp -o $(BIN)main.o

Alpha_Blending.o:
	$(CC) -c -msse4.2 $(OPT) $(MODE) $(SRC)Alpha_Blending.cpp -o $(BIN)Alpha_Blending.o

run:
	.\bin\Alpha_Blending.exe $(FR) $(BK)

clean:
	del $(BIN)Alpha_Blending.exe
