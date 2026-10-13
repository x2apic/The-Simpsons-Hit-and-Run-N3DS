#ifndef PDDI_PICA_SWIZZLE_HPP
#define PDDI_PICA_SWIZZLE_HPP

#include <stddef.h>

void picaSwizzle(void* dst, const void* src, int w, int h, int bpp);

unsigned picaSwizzleOffset(int x, int y, int w);
#endif
