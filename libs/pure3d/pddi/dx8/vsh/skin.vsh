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
dp4 r0.x, v0, c[a0.x + 17]
dp4 r0.y, v0, c[a0.x + 18]
dp4 r0.z, v0, c[a0.x + 19]

; calculate weighting
mul r10, v1.x, r0

; Transform normal for world0 matrix
dp3 r11.x, v3, c[a0.x + 17]
dp3 r11.y, v3, c[a0.x + 18]
dp3 r11.z, v3, c[a0.x + 19]
mov r11.w, c0.y

;------------------------------------------------------------------------------
; Joint 2
;------------------------------------------------------------------------------

; get palette index for joint 2 
mov a0.x, r9.y

; transform weighted joint 1 to camera space
dp4 r0.x, v0, c[a0.x + 17]
dp4 r0.y, v0, c[a0.x + 18]
dp4 r0.z, v0, c[a0.x + 19]

; calculate weighting and sum 
mad r10, v1.y, r0, r10

; Transform normal for world1 matrix
dp3 r0.x, v3, c[a0.x + 17]
dp3 r0.y, v3, c[a0.x + 18]
dp3 r0.z, v3, c[a0.x + 19]
mov r0.w, c0.y

; Lerp the two normals r11 and r0 into r11
mul r2, r11, v1.x     ; v0 * weight
add r4, c0.y, -v1.x   ; r2 = 1 - weight
mad r11, r0, r4, r11   ; normal = (1-weight)*v1 + v0*weight
; Normalize r11
dp3 r11.w, r11, r11
rsq r11.w, r11.w       
mul r11, r11, r11.w  


;------------------------------------------------------------------------------
; Joint 3
;------------------------------------------------------------------------------

; get palette index for joint 3
mov a0.x, r9.z

; 1 - weight[0] - weight[1]
; (mult by 255 needed 'cause of colour conversion)
add r0.x, v1.x, v1.y
sub r6.x, c0.y, r0.x

; transform weighted joint 1 to camera space
dp4 r0.x, v0, c[a0.x + 17]
dp4 r0.y, v0, c[a0.x + 18]
dp4 r0.z, v0, c[a0.x + 19]

; calculate weighting and sum 
 mad r10, r6.x, r0, r10


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
;-----------------------------------------------------------------------------
; Just copy the texture coordinates
mov oT0,  v7
