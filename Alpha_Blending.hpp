#ifndef ALPHA_BLENDING_INCLUDED
#define ALPHA_BLENDING_INCLUDED

#include "../../TX/TXLib.h"
#include <emmintrin.h>
#include <time.h>

#define MEASURE 1
#define N_FRAMES 10000
#define OPTIMIZED 1

#define VERT_SIZE 600
#define HOR_SIZE  800

void Draw (const char *front_name, const char *back_name);

#endif