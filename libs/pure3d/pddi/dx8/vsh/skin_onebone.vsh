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

mov r10.w, c0.y

;------------------------------------------------------------------------------
; Object -> Camera
;------------------------------------------------------------------------------

; get palette index for joint from v2 (mult by 255 needed 'cause of colour conversion)
mul r0.x, v2.x, c0.z
mov a0.x, r0.x

; transform joint to camera space
dp4 r10.x, v0, c[a0.x + 17]
dp4 r10.y, v0, c[a0.x + 18]
dp4 r10.z, v0, c[a0.x + 19]

; Transform normal 
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

mov oFog.x, r10.z


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
;-----------------------------------------------------------------------------
; Just copy the texture coordinates
mov oT0,  v7
