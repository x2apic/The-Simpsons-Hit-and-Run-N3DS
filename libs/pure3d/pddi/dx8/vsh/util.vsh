;------------------------------------------------------------------------------
; Definitions and macros for vertex shaders
;------------------------------------------------------------------------------

; assignments of vertex input registers for fixed function vertex shader
#define V_POSITION     v0
#define V_WEIGHTS      v1
#define V_INDICES      v2
#define V_NORMAL       v3
#define V_DIFFUSE      v5
#define V_SPECULAR     v6
#define V_UV0          v7
#define V_UV1          v8
#define V_UV2          v9
#define V_UV3          v10


macro	Normalize reg
   dp3 %reg.w, %reg, %reg    
   rsq %reg.w, %reg.w       
   mul %reg, %reg, %reg.w    
endm

macro Length reg 
   dp3 %reg.w, %reg, %reg
   rsq %reg.w, %reg.w
   rcp %reg.w, %reg.w
endm

macro DirLightSpecHeadlight out_diffuse, out_specular, material_diffuse, material_specular, vertex_normal, light_direction, light_colour
	;Get the material power
   mov r0.w, %material_specular

   ;Dot normal with light vector, for headlight use a constant
   ;This is the intensity of the diffuse component, 
   dp3 r0.x, %vertex_normal, -c0.xxyx;-%light_direction

   ; This is the intensity of the specular component
   ; Calculate half vector (light vector + constant eye vector)

   ;Force specular light to come from camera
   dp3 r0.yz, %vertex_normal, -c0.xxyx	

   ; Calculate the diffuse & specular factors
   lit r1, r0

  ; add the (diffuse color * diffuse light color * diffuse intensity(r1.y))
   mul r0, r1.y, %light_colour
   max r0, _ZERO, r0
   mad %out_diffuse, r0, %material_diffuse, %out_diffuse

   ; specular (specular color * specular light color * specular intensity(r1.z))
   mul r2, r1.z, %light_colour
   max r2, _ZERO, r2
   mad %out_specular, r2, %material_specular, %out_specular
endm

macro DirLightSpec out_diffuse, out_specular, material_diffuse, material_specular, vertex_normal, light_direction, light_colour
   ; Get the material power
   mov r3.w, %material_specular.w

   ; Dot normal with light vector
   ; This is the intensity of the diffuse component
   dp3 r3.x, %vertex_normal, -%light_direction;

   ; This is the intensity of the specular component
   ; Calculate half vector (light vector + constant eye vector)
   mov r0, -%light_direction
   add r0, r0, -c0.xxyx

   ; normalize half-vector
   dp3 r0.w, r0, r0
   rsq r0.w, r0.w												
   mul r0, r0, r0.w	

   ; Dot normal with half-vector.  
   dp3 r3.yz, %vertex_normal, r0

   ; Calculate the diffuse & specular factors
   lit r4, r3

   ; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
   mul r0, r4.y, %light_colour
   max r0, _ZERO, r0
   mad %out_diffuse, r0, %material_diffuse, %out_diffuse

   ; specular (specular color * specular light color * specular intensity(R4.z))
   mul r1, r4.z, %light_colour
   max r1, _ZERO, r1
   mad %out_specular, r1, %material_specular, %out_specular
endm

macro DirLight out_diffuse, out_specular, material_diffuse, material_specular, vertex_normal, light_direction, light_colour
   ; Dot normal with light vector
   ; This is the intensity of the diffuse component
   dp3 r0, %vertex_normal, -%light_direction

   ; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
   mul r1, r0.x, %light_colour
   max r1, _ZERO, r1
   mad %out_diffuse, r1, %material_diffuse, %out_diffuse
endm

;Trashes r0-r3
macro PointLight out_diffuse, out_specular, material_diffuse, material_specular, vertex_normal, vertex_position, light_position, light_colour
	; Compute the light (vertex to eye) vector
	add  r0, %light_position, -%vertex_position ; Lighting vector, L
	dp3  r1.x, r0, r0            ; d^2
	rsq  r1.z, r1.x              ; 1 / d
	mul  r0, r0, r1.z            ; Normalized L

	;A full, attenuated, colored point light
	dp3  r2.x, r0, %vertex_normal; L dot N
	max  r3, r2.x, c0.x          ; max( L dot N )

;BUG! doing something wierd to d
	rcp  r1.y, r1.x              ; 1 / d^2
	mul  r3, r3, r1.y            ; Times light attenuation
	mul  r3, r3, %light_colour   ; Times light color
    max  r3, _ZERO, r3
	mad %out_diffuse, r3, %material_diffuse, %out_diffuse
endm

;Computes a point light with full attenuation, as in:
;       1
;-----------------
;a + b*d + (d^2)*c
macro PointLightAttenuated out_diffuse, out_specular, material_diffuse, material_specular, vertex_normal, vertex_position, light_position, light_colour, light_attenuation

	; Compute the light (vertex to eye) vector and some distance factors
	add  r0, %light_position, -%vertex_position ; Lighting vector, L
	dp3  r1.w, r0, r0							; d^2
	rsq  r0.w, r1.w							    ; 1 / d

	; Compute the attenuation
	dst r1, r1.wwww, r0.wwww					; (1, d, d*d, 1/d)
	dp3 r1.w, r1, %light_attenuation			; (a0 + a1*d + a2*d2)
	rcp r1.w, r1.w								; 1/(a0 + a1*d + a2*d2)

	;Normalize the light to vertex vector
	mul  r0, r0, r0.w							; Normalized L

	; Compute the light, clamp it to zero
	dp3  r2.x, r0, %vertex_normal				; L dot N
	max  r3, r2.x, c0.x							; max( L dot N )

	; Accumulate the colour information
	mul  r3, r3, r1.w            ; Times light attenuation
	mul  r3, r3, %light_colour   ; Times light color
    max  r3, _ZERO, r3
	mad  %out_diffuse, r3, %material_diffuse, %out_diffuse

endm