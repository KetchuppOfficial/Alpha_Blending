#ifndef ALPHA_BLENDING_INCLUDED
#define ALPHA_BLENDING_INCLUDED

#include "../../TX/TXLib.h"
#include <emmintrin.h>

#define VERT_SIZE 600
#define HOR_SIZE  800

void Draw (const char *front_name, const char *back_name);
void Draw_Unoptimized (const char *front_name, const char *back_name);

#endif