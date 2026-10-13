if(SRR2_USE_BINK)
	set(BINK_SOURCES
		"../sdks/bink/binkacd.cpp"
		"../sdks/bink/binkasyncRR.c"
		"../sdks/bink/binkasyncthread.c"
		"../sdks/bink/binkcomm.c"
		"../sdks/bink/binkfile.c"
		"../sdks/bink/binkmarkers.c"
		"../sdks/bink/binkread.c"
		"../sdks/bink/binkoal.c"
		"../sdks/bink/binkssmix.c"
		"../sdks/bink/binkutil.c"
		"../sdks/bink/blitter.c" # custom
		"../sdks/bink/expand2.c"
		"../sdks/bink/popmal.c"
		"../sdks/bink/radfft.cpp"
	)

	if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
		list(APPEND BINK_SOURCES "../sdks/bink/x86_cpu.c")
	endif ()

	if (WIN32)
		list(APPEND BINK_SOURCES
			"../sdks/bink/binkplugin.c"
			"../sdks/bink/binkpluginw32.c"
			"../sdks/bink/binkxa2.cpp"
			"../sdks/bink/win32_radmem.c"
			"../sdks/bink/win32_rrAtomics.cpp"
			"../sdks/bink/win32_rrCpu.c"
			"../sdks/bink/win32_rrThreads2.c"
			"../sdks/bink/wintimer.c"
		)
		#if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
		#	list(APPEND BINK_SOURCES "../sdks/bink/win64_rrAtomics_x64.asm")
		#endif()
	else()
		list(APPEND BINK_SOURCES
			"../sdks/bink/binklnx.c"
			#"../sdks/bink/binkpa.c"
			#"../sdks/bink/binkplugin.c"
			#"../sdks/bink/binkpluginlnx.c"
			"../sdks/bink/binkssmix.c"
			"../sdks/bink/posix_defread.c"
			"../sdks/bink/posix_radmem.c"
			"../sdks/bink/posix_rrCpu.c"
			"../sdks/bink/posix_rrThreads2.c"
			"../sdks/bink/cpp_rratomics.cpp"
		)
	endif()

	add_library(bink STATIC ${BINK_SOURCES})
	target_compile_definitions(bink
		PRIVATE
			USING_EGT
			INC_BINK2
			NO_BINK20
			NO_BINK10_SUPPORT
		PUBLIC
			__RADFINAL__
			__RADINSTATICLIB__
	)
	target_link_libraries(bink ${OPENAL_LIBRARY})
	if (WIN32)
		target_link_libraries(bink Winmm)
	endif()
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(bink
			PRIVATE -Wno-multichar
		)
	endif()
	target_include_directories(bink
		PUBLIC "../sdks/bink/"
	)

	set(RADMOVIE_SOURCES
		"src/common/binkmovieplayer.cpp"
		"src/common/binkradfile.cpp"
		"src/common/binkrenderstrategy.cpp"
		"src/pch/pch.cpp"
	)
elseif(NINTENDO_3DS)
	if(SRR2_3DS_MOVIES)
		set(RADMOVIE_SOURCES
			"src/ctr/ctrmovieplayer.cpp"
			"src/common/binkrenderstrategy.cpp"
			"src/pch/pch.cpp"
		)
	else()
		set(RADMOVIE_SOURCES
			"src/common/nullmovieplayer.cpp"
			"src/common/binkrenderstrategy.cpp"
			"src/pch/pch.cpp"
		)
	endif()
else()
	set(RADMOVIE_SOURCES
		"src/common/ffmpegmovieplayer.cpp"
		"src/common/binkrenderstrategy.cpp"
		"src/pch/pch.cpp"
	)
endif()

add_library(radmovie STATIC ${RADMOVIE_SOURCES})
target_include_directories(radmovie
	PUBLIC "inc/"
	PRIVATE "src/pch/"
	PRIVATE ${OPENAL_INCLUDE_DIR}
)
target_link_libraries(radmovie
	p3d
	radcore
	radsound
)
if(SRR2_USE_BINK)
	target_link_libraries(radmovie bink)
elseif(NOT NINTENDO_3DS)
	target_link_libraries(radmovie PkgConfig::FFmpeg)
endif()
if(SRR2_BUILD_TESTS)
	add_executable(simplemovie "sample/src/main.cpp" "sample/src/util.cpp"
		"${CMAKE_SOURCE_DIR}/code/main/tuidunaligned.cpp")
	target_link_libraries(simplemovie
		radmovie
		${SDL_LIBRARY}
		${OPENAL_LIBRARY}
		p3d
		pddi
		radcore
		radsound
		PkgConfig::FFmpeg
	)
	if(VITA)
		if(SRR2_USE_VITAGL)
			target_link_libraries(simplemovie
				vitaGL
				vitashark
			)
		endif()
		target_link_libraries(simplemovie
			SceLibKernel_stub
			ScePvf_stub
			mathneon
			SceAudio_stub
			SceAppMgr_stub
			SceAppUtil_stub
			ScePgf_stub
			SceShaccCgExt
			SceShaccCg_stub
			SceCtrl_stub
			taihen_stub
			SceKernelDmacMgr_stub
		)
	endif()
	file(COPY "sample/bin/binkcd/sample_bink.rmv"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
	if(VITA)
		vita_create_self(simplemovie.self simplemovie)
	endif()
endif()
