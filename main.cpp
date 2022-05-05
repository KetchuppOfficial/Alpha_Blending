#include "Alpha_Blending.hpp"

int main (int argc, char *argv[])
{         
    if (argc < 3)
    {
        printf ("You forgot to write names of the images to blend\n");
        exit (EXIT_FAILURE);
    }
    
    #if OPTIMIZED == 1
    Draw_Optimized (argv[1], argv[2]);
    #else
    Draw_Unoptimized (argv[1], argv[2]);
    #endif
}
