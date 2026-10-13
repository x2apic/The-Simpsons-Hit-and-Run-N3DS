//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RADMATH_QUATERNION_HPP
#define _RADMATH_QUATERNION_HPP

#include <radmath/buildconfig.hpp>

#ifdef RAD_PS2
#include <radmath/platform/ps2/vu0microcode.hpp>
#endif

namespace RadicalMathLibrary
{

// forward declarations
class Matrix;
class Vector;

//-------------------------------------------------------------------
// Quaternions
//-------------------------------------------------------------------
class Quaternion
{
public:
    Quaternion() { /**/ }
    Quaternion(float W, float X, float Y, float Z) : w(W), x(X), y(Y), z(Z) {}

    void Identity(void);

    void Add(const Quaternion& q);
    void Add(const Quaternion& q1, const Quaternion& q2); 
    void Sub(const Quaternion& q);
    void Sub(const Quaternion& q1, const Quaternion& q2); 
    void Mult(const Quaternion& q);
    void Mult(const Quaternion& q1, const Quaternion& q2);
#ifdef RAD_PS2
#ifdef PS2MW
    // 32 cycles , Mult is 103 cycles. 3x speedup!
    inline void MultFast(const Quaternion *q1, const Quaternion *q2)
    {
        unsigned register temp1, temp2;
        asm
        {
            lq      temp1, 0x00(q1)         // Load the data
            lq      temp2, 0x00(q2)

            mtsab   r0, (16 - 4)            // rotate amount set to 12 bytes
           
            qfsrv   temp1, temp1, temp1     // rotate V1 by 12 bytes to the right (wxyz to xyzw)
            qfsrv   temp2, temp2, temp2     // rotate V2 by 12 bytes to the right (wxyz to xyzw)

		    qmtc2   temp1, vf1
            qmtc2   temp2, vf2
		    vcallms vu0_quaternionmultiply
            mtsab   r0, 4                   // rotate amount set to 4 bytes
		    qmfc2.i temp1, vf1		
            qfsrv   temp1, temp1, temp1     // rotate V1 by 4 bytes to the right (xyzx to wxyz)

            sq      temp1, 0x00(this)       // Store it locally
        }
    }
#else
    // 32 cycles , Mult is 103 cycles. 3x speedup!
    inline void MultFast(const Quaternion *q1, const Quaternion *q2)
    {
        QWvec v1,v2;

        // sep7/2002 amb - TODO - optimize this out!!!! WHY THE HELL DO 
        // WE STORE QUATERNIONS IN (w,x,y,z)?!?!? It should be (x,y,z,w)
        v1.fVec[0] = q1->x;
        v1.fVec[1] = q1->y;
        v1.fVec[2] = q1->z;
        v1.fVec[3] = q1->w;

        v2.fVec[0] = q2->x;
        v2.fVec[1] = q2->y;
        v2.fVec[2] = q2->z;
        v2.fVec[3] = q2->w;

	    asm __volatile__		
	    (
		    R"(.set noreorder
		    qmtc2  %0,vf1
            qmtc2  %1,vf2
		    vcallms vu0_quaternionmultiply
		    qmfc2.i  %0,vf1		
		    .set reorder)"
		    :"+r"(v1.v128) , "+r"(v2.v128)
            :
            :"vf1","vf2"
	    );

        x = v1.fVec[0];
        y = v1.fVec[1];
        z = v1.fVec[2];
        w = v1.fVec[3];
    }
#endif
#endif

    float DotProduct(const Quaternion& q) const;
    void Inverse();
    void Normalize();

    void BuildFromAngleAxis(const float a, const Vector& axis);
    void BuildFromAngleX(const float a);
    void BuildFromAngleY(const float a);
    void BuildFromAngleZ(const float a);
    void BuildFromAngleXYZ(const float x, const float y, const float z);
    void BuildFromAngleYZX(const float x, const float y, const float z);
    void BuildFromMatrix(const Matrix& m);
    
    void ConvertToMatrix(Matrix* m) const;
#if 0
#ifdef RAD_PS2
    inline void ConvertToMatrixFast(Matrix* m) const
    {
        // busted!!!
        QWvec q;

        // sep7/2002 amb - TODO - optimize this out!!!! WHY THE HELL DO 
        // WE STORE QUATERNIONS IN (w,x,y,z)?!?!? It should be (x,y,z,w)
        q.fVec[0] = x;
        q.fVec[1] = y;
        q.fVec[2] = z;
        q.fVec[3] = w;

        float* a = (float*)m;

	    asm __volatile__		
	    (
		    R"(.set noreorder
		    qmtc2  %0,vf1
            qmtc2  %1,vf2
		    vcallms vu0_quaternionmatrix
		    qmfc2.i  %1,vf24
            qmfc2    %2,vf25
            qmfc2    %3,vf26
		    .set reorder)"
		    :"+r"(q.v128) ,"+r"(a[0]),"+r"(a[4]),"+r"(a[8])
            :
            : "vf1","vf2","vf24","vf25","vf26"
	    );
    }
#endif
#endif

    // modifies this so it is on same side of hypersphere as q
    void MakeClosest(const Quaternion& q);

    // spherical-linear interpolation
    void Slerp(const Quaternion& q1, const Quaternion& q2, const float t);
    void Slerp(const Quaternion& q, const float t) { Slerp(*this, q, t); }
    
    float w, x, y, z;
};

}

namespace rmt = RadicalMathLibrary;

#endif
