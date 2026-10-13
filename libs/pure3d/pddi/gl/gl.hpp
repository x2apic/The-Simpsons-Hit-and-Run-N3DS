//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


// stub OpenGL header, all pddi gl code uses this instead of '#include <GL/gl.h>

#ifdef RAD_VITAGL
#include <vitaGL.h>
#define GL_BGRA_EXT GL_BGRA
#else
#include <glad/glad.h>
#endif

#ifdef RAD_GLES
#undef glOrtho
#undef glFrustum
#undef glClearDepth
#undef glDepthRange
#undef glFogi
#define glOrtho glOrthof
#define glFrustum glFrustumf
#define glClearDepth glClearDepthf
#define glDepthRange glDepthRangef
#define glFogi glFogx
#endif
