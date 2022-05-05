CC = g++

SRC = Source/

OPT =

all: Alpha_Blending

Alpha_Blending: main.o Alpha_Blending.o
	$(CC) main.o Alpha_Blending.o -o Alpha_Blending.exe
	del main.o Alpha_Blending.o

main.o:
	$(CC) -c $(SRC)main.cpp -o main.o

Alpha_Blending.o:
	$(CC) -c -msse4.2 $(OPT) $(SRC)Alpha_Blending.cpp -o Alpha_Blending.o

run:
	.\Alpha_Blending.exe $(FR) $(BK)

clean:
	del *.exe
