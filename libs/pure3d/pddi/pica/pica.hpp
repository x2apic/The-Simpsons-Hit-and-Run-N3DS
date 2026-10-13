#ifndef PDDI_PICA_HPP
#define PDDI_PICA_HPP

#include <3ds.h>

extern "C" int g_picaMipmapsAllowed;
extern "C" int g_picaKeepFullSize;

const unsigned PICA_MIPMAP_LINEAR_RESERVE = 6 * 1024 * 1024;

#include <citro3d.h>

#include <pddi/pddi.hpp>
#include <pddi/base/basecontext.hpp>
#include <pddi/base/baseshader.hpp>

const int PICA_SCREEN_WIDTH  = 400;
const int PICA_SCREEN_HEIGHT = 240;

const int PICA_BOTTOM_WIDTH  = 320;
const int PICA_BOTTOM_HEIGHT = 240;

const int PICA_MAX_TEXTURE_DIMENSION = 1024;
const int PICA_MIN_TEXTURE_DIMENSION = 8;

const unsigned PICA_VERTEX_STRIDE   = 36;
const unsigned PICA_VERTEX_OFF_POS  = 0;
const unsigned PICA_VERTEX_OFF_NRM  = 12;
const unsigned PICA_VERTEX_OFF_UV   = 24;
const unsigned PICA_VERTEX_OFF_COL  = 32;

extern "C" int      g_picaInFrame;

extern "C" void (*g_picaYieldHook)(void);

class picaDevice;
class picaDisplay;
class picaContext;
class picaTexture;
class picaMat;
#endif
