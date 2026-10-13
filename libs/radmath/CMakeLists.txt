set(RADMATH_SOURCES
	"radmath/matrix.cpp"
	"radmath/numerical.cpp"
	"radmath/plane.cpp"
	"radmath/quaternion.cpp"
	"radmath/radmath.cpp"
	"radmath/random.cpp"
	"radmath/spline.cpp"
	"radmath/triangle.cpp"
	"radmath/trig.cpp"
	"radmath/vector.cpp"
)
if(VITA)
	list(APPEND RADMATH_SOURCES
		"radmath/platform/vita/neonmatrix.cpp"
		"radmath/platform/vita/neonvector.cpp"
	)
endif()

add_library(radmath STATIC ${RADMATH_SOURCES})
target_include_directories(radmath
	PUBLIC "."
)
if(VITA)
	target_link_libraries(radmath mathneon)
endif()
