![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)

# Alpha Blending

## General Information

This project is a small program that is a part of the course in programming and computer architecture by [Ilya Dedinsky (aka ded32)](https://github.com/ded32) that he teaches in MIPT. This very program is one half of the task connected with using SSE and AVX optimizations. The second one is drawing [Mandelbrot set](https://github.com/KetchuppOfficial/Mandelbort_Set).

## Dependencies

Visualization of pictures is implemented with the help of [TXLib](https://github.com/ded32/TXLib) by [ded32](https://github.com/ded32). You have to install this library if you want to blend some pictures by means of my project.

## Build and run

This project is implemented for Windows only.

First of all, download this repository:
```bash
git clone git@github.com:KetchuppOfficial/Alpha_Blending.git
cd Alpha_Blending
```

Compile the program using the tool **make**:
```bash
make
```

You can also choose comliler oftimization flag as in the example below:
```bash
make OPT=-O2
```

To run the program, use **make** again:
```bash
make run FR=Images\Cat.bmp BK=Images\Table.bmp
```
FR - picture that is supposed to be on the foreground, BK - picture to place on the background.


