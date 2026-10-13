#include <pddi/pica/swizzle.hpp>

#include <string.h>

static const unsigned char tileOrder[64] =
{
     0,  1,  4,  5, 16, 17, 20, 21,
     2,  3,  6,  7, 18, 19, 22, 23,
     8,  9, 12, 13, 24, 25, 28, 29,
    10, 11, 14, 15, 26, 27, 30, 31,
    32, 33, 36, 37, 48, 49, 52, 53,
    34, 35, 38, 39, 50, 51, 54, 55,
    40, 41, 44, 45, 56, 57, 60, 61,
    42, 43, 46, 47, 58, 59, 62, 63
};

unsigned picaSwizzleOffset(int x, int y, int w)
{
    const int tileX = x >> 3;
    const int tileY = y >> 3;
    const int tilesPerRow = w >> 3;

    return (unsigned)(((tileY * tilesPerRow) + tileX) * 64)
         + tileOrder[((y & 7) << 3) | (x & 7)];
}

template< class T >
static void SwizzleTyped(T* dst, const T* src, int w, int h)
{
    const int tilesPerRow = w >> 3;

    for(int tileY = 0; tileY < (h >> 3); tileY++)
    {
        for(int tileX = 0; tileX < tilesPerRow; tileX++)
        {
            T* tile = dst + (((tileY * tilesPerRow) + tileX) * 64);
            const T* row = src + (tileY * 8 * w) + (tileX * 8);

            for(int y = 0; y < 8; y++)
            {
                const unsigned char* order = &tileOrder[y << 3];
                tile[order[0]] = row[0];
                tile[order[1]] = row[1];
                tile[order[2]] = row[2];
                tile[order[3]] = row[3];
                tile[order[4]] = row[4];
                tile[order[5]] = row[5];
                tile[order[6]] = row[6];
                tile[order[7]] = row[7];
                row += w;
            }
        }
    }
}

static void Swizzle24(unsigned char* dst, const unsigned char* src, int w, int h)
{
    const int tilesPerRow = w >> 3;

    for(int tileY = 0; tileY < (h >> 3); tileY++)
    {
        for(int tileX = 0; tileX < tilesPerRow; tileX++)
        {
            unsigned char* tile = dst + (((tileY * tilesPerRow) + tileX) * 64 * 3);
            const unsigned char* row = src + ((tileY * 8 * w) + (tileX * 8)) * 3;

            for(int y = 0; y < 8; y++)
            {
                const unsigned char* order = &tileOrder[y << 3];
                for(int x = 0; x < 8; x++)
                {
                    unsigned char* o = tile + (order[x] * 3);
                    const unsigned char* i = row + (x * 3);
                    o[0] = i[0];
                    o[1] = i[1];
                    o[2] = i[2];
                }
                row += w * 3;
            }
        }
    }
}

void picaSwizzle(void* dst, const void* src, int w, int h, int bpp)
{
    if((w < 8) || (h < 8) || (w & 7) || (h & 7))
    {
        memcpy(dst, src, (size_t)w * (size_t)h * (size_t)bpp);
        return;
    }

    switch(bpp)
    {
        case 1:
            SwizzleTyped< unsigned char >((unsigned char*)dst, (const unsigned char*)src, w, h);
            break;
        case 2:
            SwizzleTyped< unsigned short >((unsigned short*)dst, (const unsigned short*)src, w, h);
            break;
        case 3:
            Swizzle24((unsigned char*)dst, (const unsigned char*)src, w, h);
            break;
        case 4:
            SwizzleTyped< unsigned int >((unsigned int*)dst, (const unsigned int*)src, w, h);
            break;
        default:
            memcpy(dst, src, (size_t)w * (size_t)h * (size_t)bpp);
            break;
    }
}
