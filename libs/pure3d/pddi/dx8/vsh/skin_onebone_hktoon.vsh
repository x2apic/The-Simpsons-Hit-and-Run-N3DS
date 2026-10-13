vs.1.1
;------------------------------------------------------------------------------
; Input
;	v0		Position
;	v1		Blend Weight
;	v2		Blend Indices
;	v3		Normal
;	v7		UV0
;------------------------------------------------------------------------------
; Constants 
;    c0       = (0,1,255,0.5)
;    c2-c5    = projection matrix
;    c6       = material diffuse colour
;    c7       = material specular (power in w)
;    c8       = material emissive (emissove + ambient)
;    c9      = light 0 direction
;    c10      = light 0 colour
;    c11      = light 1 direction
;    c12      = light 1 colour
;    c13      = light 2 direction
;    c14      = light 2 colour
;    c15      = light 3 direction
;    c16      = light 3 colour
;    c17+     = matrix palette
;
; Temporaries used for semi-persistant data
;    r8    = Colour
;    r9    = Post-scale Blend Indices \ Specular
;    r10   = Blended Vertex
;    r11   = Blended Normal
;
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Vertex transform
;------------------------------------------------------------------------------
mul r9, v2, c0.z
mov r0.w, c0.y

;------------------------------------------------------------------------------
; Joint 1
;------------------------------------------------------------------------------

; get palette index for joint 1 
mov a0.x, r9.x

; transform weighted joint 1 to camera space
dp4 r10.x, v0, c[a0.x + 17]
dp4 r10.y, v0, c[a0.x + 18]
dp4 r10.z, v0, c[a0.x + 19]
mov r10.w, c0.y

; Transform normal for world0 matrix
dp3 r11.x, v3, c[a0.x + 17]
dp3 r11.y, v3, c[a0.x + 18]
dp3 r11.z, v3, c[a0.x + 19]
mov r11.w, c0.y

;------------------------------------------------------------------------------
; Projection
;------------------------------------------------------------------------------
; Transform to projection space
dp4 oPos.x, r10, c2 
dp4 oPos.y, r10, c3
dp4 oPos.z, r10, c4
dp4 oPos.w, r10, c5

;------------------------------------------------------------------------------
; Fog calculation
;------------------------------------------------------------------------------

;mov oFog.x, r10.z

; add emissive
mov r8, c8
mov r9, c0.x

;------------------------------------------------------------------------------
;  light0 calculation
;------------------------------------------------------------------------------
dp3 r0, r11, -c9
; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
mul r1, r0.x, c10
max r1, c0.x, r1
mad r8, r1, c6, r8


;------------------------------------------------------------------------------
;  light1 calculation
;------------------------------------------------------------------------------
dp3 r0, r11, -c11
; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
mul r1, r0.x, c12
max r1, c0.x, r1
mad r8, r1, c6, r8

;------------------------------------------------------------------------------
;  light2 calculation
;------------------------------------------------------------------------------
dp3 r0, r11, -c13
; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
mul r1, r0.x, c14
max r1, c0.x, r1
mad r8, r1, c6, r8

;------------------------------------------------------------------------------
;  light3 calculation
;------------------------------------------------------------------------------
dp3 r0, r11, -c15
; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
mul r1, r0.x, c16
max r1, c0.x, r1
mad r8, r1, c6, r8

min oD0, r8, c0.y   ; clamp if > 1
min oD1, r9, c0.y   ; clamp if > 1

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Just copy the texture coordinates
mov oT0,  v7

;collect the colours into one channel for the hc toon shader.
;This should really be the intensity though.
dp3 r1.x, c0.yyy, r8.xyz    ;add the colour components. Consider clamping the colour first?
dp3 r0.x, c0.yyy, c0.yyy    ;compute 1/3
rcp r0.x, r0.x              ;compute 1/3

mul r2.x, r1.x, r0.x		;multiply by 1/3
min r2.x, r2.x,c0.y			; clamp if > 1
mov oT1.x, r2.x
mov oT1.y, c0.x				;Not sure if this is really required

