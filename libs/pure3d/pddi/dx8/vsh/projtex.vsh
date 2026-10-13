;---------------------------------------------------------------------
;	vertex shader for projected texture
;---------------------------------------------------------------------
vs 1.1

;---------------------------------------------------------------------
;	Constants
;		c0  = ( 0, 0, 0 )
;		c2 - c5	= transform matrix( camera + projected )
;		c6  = projection vector
;---------------------------------------------------------------------

#define VPOS	v0
#define VCOLOUR v4
#define VNORMAL	v2


; transform the position 
dp4 r1.x, VPOS, c6
dp4 r1.y, VPOS, c2

; if the dot product of project vector and the vertex normal
; is negative, then set the oT1 as 0, otherwise, set as the
; transformed projected space coord
dp4 r2.x, VNORMAL, c0
mov r2.y, r2.x
mov r2.z, r2.x

sge r3, r2, c0
mul oT1 r3, r1

; copy colour
mov oD0, VCOLOUR

; copy position
mov oPos, v0



