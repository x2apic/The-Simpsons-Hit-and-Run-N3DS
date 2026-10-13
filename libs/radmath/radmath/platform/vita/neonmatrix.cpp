//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/radmath.hpp>
#include <assert.h>
extern "C"
{
#include <math_neon.h>
}

//#define P3D_NOASM

namespace RadicalMathLibrary
{

void Matrix::Transform(const Vector& src, Vector* dest) const
{
#ifdef P3D_NOASM
    float x,y,z;
    x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z + m[3][0];
    y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z + m[3][1];
    z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z + m[3][2];
    dest->Set(x,y,z);
#else
    float* mtx = const_cast<float*>(m[0]);
    float* v = reinterpret_cast<float*>(const_cast<Vector*>(&src));
    float* d = reinterpret_cast<float*>(const_cast<Vector*>(dest));
    asm volatile (
	"vld1.32 		{d0, d1}, [%1]			\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!		\n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]!		\n\t"	//Q3 = m+8
	"vld1.32 		{d24, d25}, [%0]!		\n\t"	//Q4 = m+12

	"vmul.f32 		q13, q9, d0[0]			\n\t"	//Q5 = Q1*Q0[0]
	"vmla.f32 		q13, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q13, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]
	"vadd.f32 		q13, q13, q12			\n\t"	//Q5 += Q3
	"vmov.f32 		q0, q13					\n\t"	//Q0 = q13

	"vst1.32 		d0, [%2]! 				\n\t"	//r2 = D24
	"fsts 			s2, [%2] 				\n\t"	//r2 = D25[0]
	:
	: "r"(m), "r"(v), "r"(d)
    : "q0", "q9", "q10","q11", "q12", "q13", "memory"
	);
#endif
}

void Matrix::Transform(const Vector4& src, Vector4* dest) const
{
    float* mtx = const_cast<float*>(m[0]);
    float* v = reinterpret_cast<float*>(const_cast<Vector4*>(&src));
    float* d = reinterpret_cast<float*>(const_cast<Vector4*>(dest));
#ifdef P3D_NOASM
    matvec4_c(mtx, v, d);
#else
    matvec4_neon(mtx, v, d);
#endif
}

void Matrix::RotateVector(const Vector& src, Vector* dest) const
{
#ifdef P3D_NOASM
    float x,y,z;;
    x = m[0][0]*src.x + m[1][0]*src.y + m[2][0]*src.z;
    y = m[0][1]*src.x + m[1][1]*src.y + m[2][1]*src.z;
    z = m[0][2]*src.x + m[1][2]*src.y + m[2][2]*src.z;
    dest->Set(x,y,z);
#else
    float* mtx = const_cast<float*>(m[0]);
    float* v = reinterpret_cast<float*>(const_cast<Vector*>(&src));
    float* d = reinterpret_cast<float*>(const_cast<Vector*>(dest));
    asm volatile (
	"vld1.32 		{d0, d1}, [%1]			\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!		\n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]!		\n\t"	//Q3 = m+8
	"vld1.32 		{d24, d25}, [%0]!		\n\t"	//Q4 = m+12

	"vmul.f32 		q13, q9, d0[0]			\n\t"	//Q5 = Q1*Q0[0]
	"vmla.f32 		q13, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q13, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]
	"vmov.f32 		q0, q13					\n\t"	//Q0 = q13

	"vst1.32 		d0, [%2]! 				\n\t"	//r2 = D24
	"fsts 			s2, [%2] 				\n\t"	//r2 = D25[0]
	:
	: "r"(m), "r"(v), "r"(d)
    : "q0", "q9", "q10","q11", "q12", "q13", "memory"
	);
#endif
}

void Matrix::Mult(const Matrix &a, const Matrix &b)
{
#ifdef P3D_NOASM
    // Mult
    m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]) + (a.m[0][2] * b.m[2][0]);
    m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]) + (a.m[0][2] * b.m[2][1]);
    m[0][2] = (a.m[0][0] * b.m[0][2]) + (a.m[0][1] * b.m[1][2]) + (a.m[0][2] * b.m[2][2]);
    m[0][3] = 0.0f;

    m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]) + (a.m[1][2] * b.m[2][0]);
    m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[1][1] * b.m[1][1]) + (a.m[1][2] * b.m[2][1]);
    m[1][2] = (a.m[1][0] * b.m[0][2]) + (a.m[1][1] * b.m[1][2]) + (a.m[1][2] * b.m[2][2]);
    m[1][3] = 0.0f;

    m[2][0] = (a.m[2][0] * b.m[0][0]) + (a.m[2][1] * b.m[1][0]) + (a.m[2][2] * b.m[2][0]);
    m[2][1] = (a.m[2][0] * b.m[0][1]) + (a.m[2][1] * b.m[1][1]) + (a.m[2][2] * b.m[2][1]);
    m[2][2] = (a.m[2][0] * b.m[0][2]) + (a.m[2][1] * b.m[1][2]) + (a.m[2][2] * b.m[2][2]);
    m[2][3] = 0.0f;

    m[3][0] = (a.m[3][0] * b.m[0][0]) + (a.m[3][1] * b.m[1][0]) + (a.m[3][2] * b.m[2][0]) + b.m[3][0];
    m[3][1] = (a.m[3][0] * b.m[0][1]) + (a.m[3][1] * b.m[1][1]) + (a.m[3][2] * b.m[2][1]) + b.m[3][1];
    m[3][2] = (a.m[3][0] * b.m[0][2]) + (a.m[3][1] * b.m[1][2]) + (a.m[3][2] * b.m[2][2]) + b.m[3][2];
    m[3][3] = 1.0f;

#else
    float* m0 = const_cast<float*>(b.m[0]);
    float* m1 = const_cast<float*>(a.m[0]);
    float* d = m[0];
    asm volatile (
	"vld1.32 		{d0, d1}, [%1]!			\n\t"	//q0 = m1
	"vld1.32 		{d2, d3}, [%1]!			\n\t"	//q1 = m1+4
	"vld1.32 		{d4, d5}, [%1]!			\n\t"	//q2 = m1+8
	"vld1.32 		{d6, d7}, [%1]			\n\t"	//q3 = m1+12
	"vld1.32 		{d16, d17}, [%0]!		\n\t"	//q8 = m0
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//q9 = m0+4
	"vld1.32 		{d20, d21}, [%0]!		\n\t"	//q10 = m0+8
	"vld1.32 		{d22, d23}, [%0]		\n\t"	//q11 = m0+12

	"vmul.f32 		q12, q8, d0[0] 			\n\t"	//q12 = q8 * d0[0]
	"vmul.f32 		q13, q8, d2[0] 			\n\t"	//q13 = q8 * d2[0]
	"vmul.f32 		q14, q8, d4[0] 			\n\t"	//q14 = q8 * d4[0]
	"vmul.f32 		q15, q8, d6[0]	 		\n\t"	//q15 = q8 * d6[0]
	"vmla.f32 		q12, q9, d0[1] 			\n\t"	//q12 += q9 * d0[1]
	"vmla.f32 		q13, q9, d2[1] 			\n\t"	//q13 += q9 * d2[1]
	"vmla.f32 		q14, q9, d4[1] 			\n\t"	//q14 += q9 * d4[1]
	"vmla.f32 		q15, q9, d6[1] 			\n\t"	//q15 += q9 * d6[1]
	"vmla.f32 		q12, q10, d1[0] 		\n\t"	//q12 += q10 * d1[0]
	"vmla.f32 		q13, q10, d3[0] 		\n\t"	//q13 += q10 * d3[0]
	"vmla.f32 		q14, q10, d5[0] 		\n\t"	//q14 += q10 * d5[0]
	"vmla.f32 		q15, q10, d7[0] 		\n\t"	//q15 += q10 * d6[0]
	"vadd.f32		q15, q11				\n\t"	//q15 += q11

	"vst1.32 		{d24, d25}, [%2]! 		\n\t"	//d = q12	
	"vst1.32 		{d26, d27}, [%2]!		\n\t"	//d+4 = q13	
	"vst1.32 		{d28, d29}, [%2]! 		\n\t"	//d+8 = q14	
	"vst1.32 		{d30, d31}, [%2]	 	\n\t"	//d+12 = q15	

	: "+r"(m0), "+r"(m1), "+r"(d) :
    : "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
	"memory"
	);
#endif
}

void Matrix::MultFull(const Matrix &a, const Matrix &b)
{
    float* m0 = const_cast<float*>(b.m[0]);
    float* m1 = const_cast<float*>(a.m[0]);
#ifdef P3D_NOASM
    matmul4_c(m0, m1, m[0]);
#else
    matmul4_neon(m0, m1, m[0]);
#endif
}

} // End namespace
