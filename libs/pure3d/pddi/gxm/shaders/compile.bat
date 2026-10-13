"%SCE_PSP2_SDK_DIR%\host_tools\bin\psp2cgc.exe" --profile sce_vp_psp2 vertex.cg
"%SCE_PSP2_SDK_DIR%\host_tools\build\bin\psp2bin.exe" vertex_cg.gxp -b2e PSP2,_binary_vertex_gxp_start,_binary_vertex_gxp_size,4 -o vertex_gxp.obj
"%SCE_PSP2_SDK_DIR%\host_tools\bin\psp2cgc.exe" --profile sce_fp_psp2 color.cg
"%SCE_PSP2_SDK_DIR%\host_tools\build\bin\psp2bin.exe" color_cg.gxp -b2e PSP2,_binary_color_gxp_start,_binary_color_gxp_size,4 -o color_gxp.obj
"%SCE_PSP2_SDK_DIR%\host_tools\bin\psp2cgc.exe" --profile sce_fp_psp2 texture.cg
"%SCE_PSP2_SDK_DIR%\host_tools\build\bin\psp2bin.exe" texture_cg.gxp -b2e PSP2,_binary_texture_gxp_start,_binary_texture_gxp_size,4 -o texture_gxp.obj
"%SCE_PSP2_SDK_DIR%\host_tools\bin\psp2cgc.exe" --profile sce_fp_psp2 alpha.cg
"%SCE_PSP2_SDK_DIR%\host_tools\build\bin\psp2bin.exe" alpha_cg.gxp -b2e PSP2,_binary_alpha_gxp_start,_binary_alpha_gxp_size,4 -o alpha_gxp.obj
pause
