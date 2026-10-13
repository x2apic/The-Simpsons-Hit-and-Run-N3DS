//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/radmath.hpp>
extern "C"
{
#include <math_neon.h>
}


//#define P3D_NOASM

namespace RadicalMathLibrary
{

// cross products
void Vector::CrossProduct(const Vector& vect)
{
    float v0[] = {x,y,z};
    float* v1 = reinterpret_cast<float*>(const_cast<Vector*>(&vect));
    float* d = reinterpret_cast<float*>(this);
#ifdef P3D_NOASM
    cross3_c(v0, v1, d);
#else
    cross3_neon(v0, v1, d);
#endif
}


void Vector::CrossProduct(const Vector& vect1, const Vector& vect2)
{
    float* v0 = reinterpret_cast<float*>(const_cast<Vector*>(&vect1));
    float* v1 = reinterpret_cast<float*>(const_cast<Vector*>(&vect2));
    float* d = reinterpret_cast<float*>(this);
#ifdef P3D_NOASM
    cross3_c(v0, v1, d);
#else
    cross3_neon(v0, v1, d);
#endif
}

// create a unit vector
void Vector::Normalize(void)
{
    float v[] = {x,y,z};
    float* d = reinterpret_cast<float*>(this);
#ifdef P3D_NOASM
    normalize3_c(v, d);
#else
    normalize3_neon(v, d);
#endif
}

void Vector::Normalize(const Vector& vect)
{
    float* v = reinterpret_cast<float*>(const_cast<Vector*>(&vect));
    float* d = reinterpret_cast<float*>(this);
#ifdef P3D_NOASM
    normalize3_c(v, d);
#else
    normalize3_neon(v, d);
#endif
}

}   // end namespace RadicalMathLibrary

