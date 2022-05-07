#include "../../../TX/TXLib.h"
#include "../include/Alpha_Blending.hpp"

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Some information about Windows specific data names, types and structures:

1) RGBQUAD
    typedef struct tagRGBQUAD
    {
        BYTE rgbBlue;     - intensity of blue
        BYTE rgbGreen;    - intensity of green
        BYTE rgbRed;      - intensity of red
        BYTE rgbReserved; - this member is reserved and must be 0
    } RGBQUAD;

2) HDC (a handle to a device context):
    typedef void *PVOID;   --->    typedef PVOID HANDLE;   --->   typedef HANDLE HDC;

3) BYTE:
    typedef unsigned char BYTE;  

4) LARGE_INTEGER: if system is 64-bit, QuadPart should be used
    typedef union _LARGE_INTEGER
    {
        struct
        {
            DWORD LowPart;
            LONG  HighPart;
        } DUMMYSTRUCTNAME;

        struct
        {
            DWORD LowPart;
            LONG  HighPart;
        } u;

        LONGLONG QuadPart;
    } LARGE_INTEGER;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

struct Img
{
    HDC image;
    int width;
    int height;
};

struct Blending
{
    RGBQUAD *front;
    HDC front_dc;
    RGBQUAD *back;
    HDC back_dc;
    int width;
    int height;
};

static Img Load_Image (const char *img_name)
{
    HDC img        = txLoadImage  (img_name);
    int img_width  = txGetExtentX (img);
    int img_height = txGetExtentY (img);

    Img img_struct = {img, img_width, img_height};
    return img_struct;
}

static Blending Load_Images (const char *foreground, const char *background)
{
    Img front = Load_Image (foreground);
    Img back  = Load_Image (background);

    if (front.width > back.width || front.height > back.height)
    {
        printf ("Foreground picutre souldn't be bigger than background picture\n");
        Blending invalid = {NULL, NULL, NULL, NULL, 0, 0};
        return invalid;
    }

    RGBQUAD *back_ptr = NULL;
    HDC back_dc = txCreateDIBSection (back.width, back.height, &back_ptr);
    txBitBlt (back_dc, 0, 0, 0, 0, back.image);
    txDeleteDC (back.image);

    RGBQUAD *front_ptr = NULL;
    HDC front_dc = txCreateDIBSection (back.width, back.height, &front_ptr);
    txBitBlt (front_dc, 0, 0, 0, 0, front_dc, 0, 0, BLACKNESS); // filling canvas with black
    //                                       /    \.
    //                                      x  and  y of left top corner of copied img inside itself

    txBitBlt (front_dc, (back.width - front.width) / 2, (back.height - front.height) / 2, 0, 0, front.image);
    //            /                  /                            \                       /   \          \.
    //    destination     x of the img after copying   y of img after copying        width     height     source
    //                                                                        if 0, then w/h == w/h of source

    txDeleteDC (front.image);

    Blending valid = {front_ptr, front_dc, back_ptr, back_dc, back.width, back.height};
    return valid;
}

static inline RGBQUAD *Create_Window (const int hor_size, const int vert_size)
{
    txCreateWindow (hor_size, vert_size);       // creates window of (hor_size x vert_size); window is centered

    RGBQUAD *screen_buff = txVideoMemory ();    // return pointer on the screen buffer as if it was one dimention array

    return screen_buff;
}

static void Blend_Optimized (RGBQUAD *front, RGBQUAD *back, RGBQUAD *screen, const int width, const int height)
{
    const __m128i   _0 = _mm_set_epi8 (0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0);
    const __m128i _255 = _mm_cvtepu8_epi16 (_mm_set_epi8 (255U, 255U, 255U, 255U,  255U, 255U, 255U, 255U, 
                                                          255U, 255U, 255U, 255U,  255U, 255U, 255U, 255U));
    
    const unsigned char high_on = 128; // 128d = 10000000b
    
    for (int y = 0; y < height; y++)
    for (int x = 0; x < width;  x += 4)
    {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 1:

        //-----------------------------------------------------------------------
        //       15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // fr = [r3 g3 b3 a3 | r2 g2 b2 a2 | r1 g1 b1 a1 | r0 g0 b0 a0]
        //-----------------------------------------------------------------------

        __m128i fr = _mm_loadu_si128 ((__m128i*)(front + y * width + x));
        __m128i bk = _mm_loadu_si128 ((__m128i*)(back  + y * width + x));
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 2:

        //-----------------------------------------------------------------------
        //       15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // fr = [a3 r3 g3 b3 | a2 r2 g2 b2 | a1 r1 g1 b1 | a0 r0 g0 b0]
        //        \  \  \  \    \  \  \  \   xx xx xx xx   xx xx xx xx            
        //         \  \  \  \    \  \  \  \.
        //          \  \  \  \    '--+--+--+-------------+--+--+--.
        //           '--+--+--+------------+--+--+--.     \  \  \  \.
        //                                  \  \  \  \     \  \  \  \.
        // FR = [-- -- -- -- | -- -- -- -- | a3 r3 g3 b3 | a2 r2 g2 b2]
        //-----------------------------------------------------------------------
        
        __m128i FR = (__m128i) _mm_movehl_ps ((__m128) _0, (__m128) fr);
        __m128i BK = (__m128i) _mm_movehl_ps ((__m128) _0, (__m128) bk);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 3:

        //-----------------------------------------------------------------------
        //       15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // fr = [a3 r3 g3 b3 | a2 r2 g2 b2 | a1 r1 g1 b1 | a0 r0 g0 b0]
        //       xx xx xx xx   xx xx xx xx                 /  /   |  |
        //                                         _______/  /   /   |
        //            ...   ...     ...           /     ____/   /    |
        //           /     /       /             /     /       /     |
        // fr = [-- a1 -- r1 | -- g1 -- b1 | -- a0 -- r0 | -- g0 -- b0]
        //-----------------------------------------------------------------------

        fr = _mm_cvtepi8_epi16 (fr);
        FR = _mm_cvtepi8_epi16 (FR);

        bk = _mm_cvtepi8_epi16 (bk);
        BK = _mm_cvtepi8_epi16 (BK);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 4: 
    
        //-----------------------------------------------------------------------
        //       15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // fr = [-- a1 -- r1 | -- g1 -- b1 | -- a0 -- r0 | -- g0 -- b0]
        //          |___________________        |___________________
        //          |     \      \      \       |     \      \      \.
        // a  = [-- a1 -- a1 | -- a1 -- a1 | -- a0 -- a0 | -- a0 -- a0]
        //-----------------------------------------------------------------------

        __m128i mask = _mm_set_epi8 (high_on, 14, high_on, 14, high_on, 14, high_on, 14,
                                     high_on,  6, high_on,  6, high_on,  6, high_on,  6);

        __m128i alpha = _mm_shuffle_epi8 (fr, mask);
        __m128i ALPHA = _mm_shuffle_epi8 (FR, mask);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 5:

        fr = _mm_mullo_epi16 (fr, alpha);   // fr *= alpha
        FR = _mm_mullo_epi16 (FR, ALPHA);   // FR *= ALPHA

        bk = _mm_mullo_epi16 (bk, _mm_sub_epi16 (_255, alpha)); // bk *= (255 - alpha)
        BK = _mm_mullo_epi16 (BK, _mm_sub_epi16 (_255, ALPHA)); // BK *= (255 - ALPHA)

        __m128i sum = _mm_add_epi16 (fr, bk);  // sum = fr + bk
        __m128i SUM = _mm_add_epi16 (FR, BK);  // SUM = FR + BK
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 6:

        //-----------------------------------------------------------------------
        //        15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // sum = [A1 a1 R1 r1 | G1 g1 B1 b1 | A0 a0 R0 r0 | G0 g0 B0 b0]
        //         \     \       \     \       \_____\_______\_____\.
        //          \_____\_______\_____\______________    \  \  \  \.
        //                                    \  \  \  \    \  \  \  \.
        // sum = [-- -- -- -- | -- -- -- -- | A1 R1 G1 B1 | A0 R0 G0 B0]
        //-----------------------------------------------------------------------

        mask = _mm_set_epi8 (high_on, high_on, high_on, high_on, high_on, high_on, high_on, high_on, 
                                 15U,     13U,     11U,      9U,      7U,      5U,      3U,      1U);

        sum = _mm_shuffle_epi8 (sum, mask);
        SUM = _mm_shuffle_epi8 (SUM, mask);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 7:
        
        //-----------------------------------------------------------------------
        //          15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // sum   = [-- -- -- -- | -- -- -- -- | a1 r1 g1 b1 | a0 r0 g0 b0] ->-.
        // sumHi = [-- -- -- -- | -- -- -- -- | a3 r3 g3 b3 | a2 r2 g2 b2]    |
        //                                      /  /  /  /    /  /  /  /      V
        //             .--+--+--+----+--+--+--++--+--+--+----+--+--+--'       |
        //            /  /  /  /    /  /  /  /    ____________________________/
        //           /  /  /  /    /  /  /  /    /  /  /  /    /  /  /  /
        // color = [a3 r3 g3 b3 | a2 r2 g2 b2 | a1 r1 g1 b1 | a0 r0 g0 b0]
        //-----------------------------------------------------------------------

        __m128i color = (__m128i) _mm_movelh_ps ((__m128) sum, (__m128) SUM);

        _mm_storeu_si128 ((__m128i*)(screen + y * width + x), color);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    }
}

static void Blend_Unoptimized (RGBQUAD *front, RGBQUAD *back, RGBQUAD *screen, const int width, const int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            RGBQUAD* fr = front + y * width + x;
            RGBQUAD* bk = back  + y * width + x;
            
            uint16_t alpha  = fr->rgbReserved;

            *(screen + y * width + x) = {(BYTE) ( (fr->rgbBlue  * alpha + bk->rgbBlue  * (255 - alpha)) >> 8 ),
                                         (BYTE) ( (fr->rgbGreen * alpha + bk->rgbGreen * (255 - alpha)) >> 8 ),
                                         (BYTE) ( (fr->rgbRed   * alpha + bk->rgbRed   * (255 - alpha)) >> 8 )};
        }
    }
}
    
void Draw (const char *front_name, const char *back_name)
{
    assert (front_name);
    assert (back_name);

    Win32::_fpreset();  // reinitialization of math coprocessor ()
                        // TXLib aborts the program if any error accures
                        // this function returns the state to the initial one 

    Blending front_back = Load_Images (front_name, back_name);
    if (front_back.front == NULL)
        return;

    RGBQUAD *front = front_back.front;
    RGBQUAD *back  = front_back.back;

    RGBQUAD *screen = Create_Window (front_back.width, front_back.height);

    #if MEASURE == 1
    double run_time = 0.0;

    LARGE_INTEGER frequency = {};
    QueryPerformanceFrequency (&frequency);
    #endif

    unsigned long long n_frames = 0;
    for (; n_frames < N_FRAMES; n_frames++)
    {
        if (txGetAsyncKeyState (VK_ESCAPE))
            break;
        
        #if MEASURE == 1
        LARGE_INTEGER start = {};
        QueryPerformanceCounter (&start);
        #endif
        
        #ifdef OPTIMIZED
        Blend_Optimized   (front, back, screen, front_back.width, front_back.height);
        #elif
        Blend_Unoptimized (front, back, screen, front_back.width, front_back.height);
        #endif

        #if MEASURE == 1
        LARGE_INTEGER finish = {};
        QueryPerformanceCounter (&finish);
        run_time += (double)(finish.QuadPart - start.QuadPart);
        #endif

        txUpdateWindow();
    }

    #if MEASURE == 1
    unsigned long long actual_frames = (n_frames < N_FRAMES - 1) ? n_frames : N_FRAMES; 
    printf ("Average FPS: %f\n", (double)actual_frames * (double)frequency.QuadPart / run_time);
    #endif

    txDisableAutoPause();

    txDeleteDC (front_back.front_dc);
    txDeleteDC (front_back.back_dc);
}
