all: Alpha_Blending

Alpha_Blending:
	g++ Alpha_Blending.cpp -msse4.2 -O3 -o Alpha_Blending.exe

run:
	.\Alpha_Blending.exe "Images\Cat.bmp" "Images\Table.bmp"

clean:
	del *.exe
