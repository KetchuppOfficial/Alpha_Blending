#include "Alpha_Blending.hpp"

typedef RGBQUAD (&scr_t) [VERT_SIZE][HOR_SIZE];

static inline scr_t Load_Image (const char* filename)
{
    RGBQUAD* mem = NULL;
    HDC dc = txCreateDIBSection (HOR_SIZE, VERT_SIZE, &mem);
    txBitBlt (dc, 0, 0, 0, 0, dc, 0, 0, BLACKNESS);

    HDC image = txLoadImage (filename);
    txBitBlt (dc, (txGetExtentX (dc) - txGetExtentX (image)) / 2, 
                  (txGetExtentY (dc) - txGetExtentY (image)) / 2, 0, 0, image);

    txDeleteDC (image);

    return (scr_t) *mem;
}

static void Calculate (scr_t front, scr_t back, scr_t screen)
{
    const __m128i   _0 = _mm_set_epi8 (0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0);
    const __m128i _255 = _mm_cvtepu8_epi16 (_mm_set_epi8 (255U, 255U, 255U, 255U,  255U, 255U, 255U, 255U, 
                                                          255U, 255U, 255U, 255U,  255U, 255U, 255U, 255U));
    
    const unsigned char high_on = 128; // 128d = 10000000b
    
    for (int y = 0; y < VERT_SIZE; y++)
    for (int x = 0; x < HOR_SIZE;  x += 4)
    {
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 1:

        //-----------------------------------------------------------------------
        //       15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
        // fr = [r3 g3 b3 a3 | r2 g2 b2 a2 | r1 g1 b1 a1 | r0 g0 b0 a0]
        //-----------------------------------------------------------------------

        __m128i fr = _mm_loadu_si128 ((__m128i*) &front[y][x]);
        __m128i bk = _mm_loadu_si128 ((__m128i*) &back[y][x]);
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

        const __m128i a_mask = _mm_set_epi8 (high_on, 14, high_on, 14, high_on, 14, high_on, 14,
                                             high_on,  6, high_on,  6, high_on,  6, high_on,  6);

        __m128i a = _mm_shuffle_epi8 (fr, a_mask);
        __m128i A = _mm_shuffle_epi8 (FR, a_mask);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // STEP 5:

        fr = _mm_mullo_epi16 (fr, a);   // fr *= a
        FR = _mm_mullo_epi16 (FR, A);   // FR *= A

        bk = _mm_mullo_epi16 (bk, _mm_sub_epi16 (_255, a)); // bk *= (255 - a)
        BK = _mm_mullo_epi16 (BK, _mm_sub_epi16 (_255, A)); // BK *= (255 - A)

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

        const __m128i sum_mask = _mm_set_epi8 (high_on, high_on, high_on, high_on, high_on, high_on, high_on, high_on, 
                                                   15U,     13U,     11U,      9U,      7U,      5U,      3U,      1U);

        sum = _mm_shuffle_epi8 (sum, sum_mask);
        SUM = _mm_shuffle_epi8 (SUM, sum_mask);
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

        _mm_storeu_si128 ((__m128i*) &screen[y][x], color);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    }
}

static void Blend (scr_t front, scr_t back, scr_t screen)
{
    for (int y = 0; y < VERT_SIZE; y++)
    {
        for (int x = 0; x < HOR_SIZE; x++)
        {
            RGBQUAD* fr = &front[y][x];
            RGBQUAD* bk = &back [y][x];
            
            uint16_t a  = fr->rgbReserved;

            screen[y][x] = {(BYTE) ( (fr->rgbBlue  * a + bk->rgbBlue  * (255 - a)) >> 8 ),
                            (BYTE) ( (fr->rgbGreen * a + bk->rgbGreen * (255 - a)) >> 8 ),
                            (BYTE) ( (fr->rgbRed   * a + bk->rgbRed   * (255 - a)) >> 8 )};
        }
    }
}

static void Create_Window (const int hor_size, const int vert_size)
{
    txCreateWindow (hor_size, vert_size);
    Win32::_fpreset();
    txBegin();
}
    
void Draw (const char *front_name, const char *back_name)
{
    Create_Window (HOR_SIZE, VERT_SIZE);
    
    scr_t front  = Load_Image (front_name);
    scr_t back   = Load_Image (back_name);
    scr_t screen = (scr_t) *txVideoMemory();

    for (int n = 0; ; n++)
    {
        if (GetAsyncKeyState (VK_ESCAPE))
            break;
        
        if (!GetKeyState (VK_CAPITAL))
            Calculate (front, back, screen);
        else
            Blend (front, back, screen);
                
        if (!(n % 10))
            printf ("\t\r%.0lf", txGetFPS() * 10);

        txUpdateWindow();
    }

    txDisableAutoPause();
}