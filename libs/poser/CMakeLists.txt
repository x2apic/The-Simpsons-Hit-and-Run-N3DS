set(POSER_SOURCES
	"src/joint.cpp"
	"src/pose.cpp"
	"src/posedriver.cpp"
	"src/poseengine.cpp"
	"src/transform.cpp"
)

add_library(poser STATIC ${POSER_SOURCES})
target_include_directories(poser
	PUBLIC "inc/"
)
target_link_libraries(poser
	radcore
	p3d
	radmath
)
