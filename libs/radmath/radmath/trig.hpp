//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_TRIG_HPP
#define _RADMATH_TRIG_HPP

#include <radmath/buildconfig.hpp>
#include <math.h>
#include <radmath/util.hpp>

#ifdef RAD_PS2
#include <radmath/platform/ps2/vu0microcode.hpp>
#endif

namespace RadicalMathLibrary
{

// trig stuff
const float PI     = 3.14159265358979323846f;
const float PI_2   = PI * 2.0f;
const float PI_BY2 = PI / 2.0f;
const float PI_BY4 = PI / 4.0f;
const float PI_BY8 = PI / 8.0f;
const float OO_PI  = 1.0f / PI;

// convert to and from old style unsigned short angles (P3DANGLE)
template <class T> inline float AngleToRadian(T a)
{
    return float(a) * (PI_2 / 65536.0f);
}

inline unsigned short RadianToAngle(const float a)
{
    return (unsigned short)(a * (65536.0f / PI_2));
}

// convert from/to radians and degrees
inline float RadianToDeg(const float a)
{
    return a * (360.0f / PI_2);
}

inline float DegToRadian(const float a)
{
    return a * (PI_2 / 360.0f);
}

#ifndef RAD_PS2
    inline float Sin(const float angle)  {  return sinf(angle); }
    inline float Cos(const float angle)  {  return cosf(angle); }
    inline void  SinCos(const float angle, float* s, float* c)
    {
        *s = sinf(angle);
        *c = cosf(angle);
    }
#endif

#ifdef RAD_PS2

    // from sce.dev.toptips , 11/9/2002 "FPU sin cos" by Colin Hughes
    // sep7/2002 amb - 51 cycles , can still be optimized to about 45 cycles. sinf() takes > 250 cycles.
#ifdef PS2MW
    inline float Sin(register float a)
    {
        register float pi_by2  = PI_BY2;
        register float oo_pi   = (OO_PI) * -0.5f;
        register float two     = 2.0F;
        register float bias    = (float)0x400000;
        register float quarter = 0.25F;

        register float kv0 =   6.283185f;
        register float kv1 = -41.34167f;
        register float kv2 =  81.60223f;
        register float kv3 = -76.57496f;
        register float kv4 =  39.71066f;

        register float fv1, fv2, fv24, fv25;
        asm 
        {
            sub.s   fv1, a, pi_by2      // Subtract PI/2 from angle: NOT NEEDED FOR COSF
            abs.s   fv1, fv1            // Mirror around zero
            mula.s  fv1, oo_pi          // Multiply by -1/2PI for cycle in 0 to -1
            msuba.s two, bias           // Add bias value 2*max
            madda.s two, bias           // Subtract bias value - This will leave negative integer value only in ACC
            msuba.s fv1, oo_pi          // Fractional part only 0 - 1 range
            msub.s  fv1, two, quarter   // Subtract 0.5 so range is -0.5 to +0.5 (2*0.25)
            abs.s   fv1, fv1            // Mirror around zero
            sub.s   fv1, fv1, quarter   // Sub 0.25, range now -0.25 to +0.25

            mul.s   fv2,  fv1, fv1      // A^2
            mula.s  fv1,  kv0           // k0 A
            mul.s   fv24, fv1,  fv2     // A^3
            mul.s   fv25, fv24, fv2     // A^5
            madda.s fv24, kv1           // + k1 A^3
            mul.s   fv24, fv25, fv2     // A^7
            madda.s fv25, kv2           // + k2 A^5
            mul.s   fv25, fv24, fv2     // A^9
            madda.s fv24, kv3           // + k3 A^7
            madd.s  fv2,  fv25, kv4     // + k4 A^9
        }
        return fv2;
    }

    /*inline float Cos(register float a)
    {
        register float oo_pi   = (OO_PI) * -0.5F;
        register float two     = 2.0F;
        register float bias    = (float)0x400000;
        register float quarter = 0.25F;
        
        register float kv0 =   6.283185f;
        register float kv1 = -41.34167f;
        register float kv2 =  81.60223f;
        register float kv3 = -76.57496f;
        register float kv4 =  39.71066f;

        float fv1, fv2, fv24, fv25;
        asm
        {
            abs.s   fv1, a              // Mirror around zero
            mula.s  fv1, oo_pi          // Multiply by -1/2PI for cycle in 0 to -1
            msuba.s two, bias           // Add bias value 2*max
            madda.s two, bias           // Subtract bias value - This will leave negative integer value only in ACC
            msuba.s fv1, oo_pi          // Fractional part only 0 - 1 range
            msub.s  fv1, two, quarter   // Subtract 0.5 so range is -0.5 to +0.5 (2*0.25)
            abs.s   fv1, fv1            // Mirror around zero
            sub.s   fv1, fv1, quarter   // Sub 0.25, range now -0.25 to +0.25

            mul.s   fv2,  fv1,  fv1     // A^2
            mula.s  fv1,  kv0           // k0 A
            mul.s   fv24, fv1,  fv2     // A^3
            mul.s   fv25, fv24, fv2     // A^5
            madda.s fv24, kv1           // + k1 A^3
            mul.s   fv24, fv25, fv2     // A^7
            madda.s fv25, kv2           // + k2 A^5
            mul.s   fv25, fv24, fv2     // A^9
            madda.s fv24, kv3           // + k3 A^7
            madd.s  fv2,  fv25, kv4     // + k4 A^9
        }
        return fv2;
    }*/
    inline float Cos(register float a)
    {
        return( cosf(a) );
    }

    // 72 cycles
    inline void SinCos(register const float angle, register float* sine, register float* cosine)
    {
        register unsigned va;
        
        asm
        {
            .set noreorder
            mtsab    r0, 0x04           // shift amount set to 4 bytes
            mfc1     va, angle          // Move float to int register
            pcpyld   va, va, va         // duplicate float to upper word
            qmtc2    va, vf1            // move to VU0 register
            vcallms  vu0_sincos         // do the work
            qmfc2.i  va, vf1            // read the results 
            sw       va, 0x00(sine)     // store the sine
            qfsrv    va, va, va         // rotate VA by 4 bytes to the right
            sw       va, 0x00(cosine)   // store the cosine
            .set reorder
        }
    }

    // WOOT! We can do 2 angles at once in 72 cycles.
    inline void SinCos2(register float angle1, register float angle2, 
                        register float* sin1, register float *cos1, 
                        register float* sin2, register float *cos2)
    {
        register unsigned  va;
        register unsigned  vb;

        asm
        {
            .set noreorder
            mtsab    r0, 0x04       // shift amount set to 4 bytes
            mfc1     va, angle1     // Move float to int register
            mfc1     vb, angle2     // Move float to int register
            pcpyld   va, va, vb     // duplicate float to upper word
            qmtc2    va, vf1        // move to VU0 register
            vcallms  vu0_sincos     // do the work
            qmfc2.i  va, vf1        // read the results 
            sw       va, 0x00(sin2) // store the sine
            qfsrv    va, va, va     // rotate VA by 4 bytes to the right
            sw       va, 0x00(cos2) // store the cosine
            qfsrv    va, va, va     // rotate VA by 4 bytes to the right
            sw       va, 0x00(sin1) // store the sine
            qfsrv    va, va, va     // rotate VA by 4 bytes to the right
            sw       va, 0x00(cos1) // store the cosine
            .set reorder
        }
    }

#else // Not PS2MW
    inline float Sin(float a)
    {
        float f1;
        asm __volatile__(R"(
        sub.s %0,%1,%2   # Subtract PI/2 from angle: NOT NEEDED FOR COSF
        abs.s %0,%0   # Mirror around zero
        mula.s %0,%3   # Multiply by -1/2PI for cycle in 0 to -1
        msuba.s %4,%5   # Add bias value 2*max
        madda.s %4,%5   # Subtract bias value - This will leave negative integer value only in ACC
        msuba.s %0,%3   # Fractional part only 0 - 1 range
        msub.s %0,%4,%6   # Subtract 0.5 so range is -0.5 to +0.5 (2*0.25)
        abs.s %0,%0   # Mirror around zero
        sub.s %0,%0,%6   # Sub 0.25, range now -0.25 to +0.25
        )" : "=&f"( f1 ) : "f"( a ),
        "f"( (float)(PI_BY2) ),
        "f"( (float)((OO_PI)*-0.5f) ),
        "f"( 2.0f ),
        "f"( (float)0x400000 ),
        "f"( 0.25f )
        );

        // Inline taylor series calculation
        float f2;
        asm __volatile__(R"(
        mul.s %0,%1,%1    # A^2
        mula.s %1,%2   # k0 A
        mul.s $f24,%1,%0  # A^3
        mul.s $f25,$f24,%0  # A^5
        madda.s $f24,%3   # + k1 A^3
        mul.s $f24,$f25,%0  # A^7
        madda.s $f25,%4   # + k1 A^5
        mul.s $f25,$f24,%0  # A^9
        madda.s $f24,%5   # + k1 A^7
        madd.s %0,$f25,%6  # + k1 A^9
        )" : "=&f"( f2 ) : "f"(f1),
        "f"( 6.283185f ), // k[0].f ),
        "f"( -41.34167f ), // k[1].f ),
        "f"( 81.60223f ), // k[2].f ),
        "f"( -76.57496f ), // k[3].f ),
        "f"( 39.71066f ) // k[4].f )
        : "$f24","$f25" );

        return f2;
    }

    inline float Cos(float a)
    {
        float f1;
        asm __volatile__(R"(
        abs.s %0,%1      # Mirror around zero
        mula.s %0,%3     # Multiply by -1/2PI for cycle in 0 to -1
        msuba.s %4,%5    # Add bias value 2*max
        madda.s %4,%5    # Subtract bias value - This will leave negative integer value only in ACC
        msuba.s %0,%3    # Fractional part only 0 - 1 range
        msub.s %0,%4,%6  # Subtract 0.5 so range is -0.5 to +0.5 (2*0.25)
        abs.s %0,%0      # Mirror around zero
        sub.s %0,%0,%6   # Sub 0.25, range now -0.25 to +0.25
        )" : "=&f"( f1 ) : "f"( a ),
        "f"( 0.0f ),
        "f"( (float)((OO_PI)*-0.5f) ),
        "f"( 2.0f ),
        "f"( (float)0x400000 ),
        "f"( 0.25f )
        );

        // Inline taylor series calculation
        float f2;
        asm __volatile__(R"(
        mul.s %0,%1,%1    # A^2
        mula.s %1,%2       # k0 A
        mul.s $f24,%1,%0   # A^3
        mul.s $f25,$f24,%0  # A^5
        madda.s $f24,%3   # + k1 A^3
        mul.s $f24,$f25,%0  # A^7
        madda.s $f25,%4   # + k1 A^5
        mul.s $f25,$f24,%0  # A^9
        madda.s $f24,%5   # + k1 A^7
        madd.s %0,$f25,%6  # + k1 A^9
        )" : "=&f"( f2 ) : "f"(f1),
        "f"( 6.283185f ), // k[0].f ),
        "f"( -41.34167f ), // k[1].f ),
        "f"( 81.60223f ), // k[2].f ),
        "f"( -76.57496f ), // k[3].f ),
        "f"( 39.71066f ) // k[4].f )
        : "$f24","$f25" );

        return f2;
    }

    // 72 cycles
    inline void SinCos(const float angle, float* s, float* c)
    {
        QWvec va;

	    asm("pcpyld %0,%1,%2" : "=r"(va.v128) : "r"(angle), "r"(angle) );
	    asm volatile		
	    (
		    R"(.set noreorder
		    qmtc2  %0,vf1
		    vcallms vu0_sincos
		    qmfc2.i  %0,vf1		
		    .set reorder)"
		    :"+r"(va.v128)  
            :
            : "vf1"
	    );
	    *c = va.fVec[1];
	    *s = va.fVec[0]; 
    }

    // WOOT! We can do 2 angles at once in 72 cycles.
    inline void SinCos2(float angle1, float angle2, float* s1, float *c1, float* s2, float *c2)
    {
        QWvec va;

	    asm("pcpyld %0,%1,%2" : "=r"(va.v128) : "r"(angle1), "r"(angle2) );
	    asm volatile		
	    (
		    R"(.set noreorder
		    qmtc2  %0,vf1
		    vcallms vu0_sincos
		    qmfc2.i  %0,vf1		
		    .set reorder)"
		    :"+r"(va.v128)  
            :
            : "vf1"
	    );

	    *s1 = va.fVec[2]; 
        *c1 = va.fVec[3];
	    
        *s2 = va.fVec[0];
	    *c2 = va.fVec[1];
    }
#endif
#endif

#if 0 //def RAD_PS2
// tanf = 171 cycles, Tan = 72 cycles    
inline float Tan(const float angle)  {  float s,c; SinCos(angle,&s,&c); return (s/c); }
#else
inline float Tan(const float angle)  {  return tanf(angle); }
#endif

inline float ASin(const float angle) {  return asinf(angle); }
inline float ACos(const float angle) {  return acosf(angle); }
inline float ATan(const float angle) {  return atanf(angle); }
inline float ATan2(const float y, const float x )  {  return atan2f(y, x); }

// This  arctan function basically takes an X/Y pair  and gives you the angle 
// between the positive X axis and the vector from the origin to that point
float ATanCoords(float x, float y);

// Polar<->Cartesian coordinate support.
void CartesianToPolar(float x, const float y, float* magnitude, float* angle );
void PolarToCartesian(float magnitude, float angle,  float* x, float* y );

// Spherical<->Cartesian support. Same details as the Polar stuff
// angledelta is rotation around Z axis (on XY plane)
// anglephi is elevation (above/below the XY plane),
void CartesianToSpherical(float x, float y, float z, float* magnitude, float* angletheta, float* anglephi );
void SphericalToCartesian(float magnitude, float angletheta, float anglephi, float* x, float* y, float* z );

}  // RadicalMathLibrary namespace

namespace rmt = RadicalMathLibrary;

#endif  //TRIG_HPP

