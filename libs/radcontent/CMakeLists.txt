set(RADCONTENT_SOURCES
	"src/radload/request.cpp"
	"src/radload/queue.cpp"
	"src/radload/hashtable.cpp"
	"src/radload/inventory.cpp"
	"src/radload/stream.cpp"
	"src/radload/manager.cpp"
	"src/radload/object.cpp"
	#"src/pch/pch.cpp"
)

add_library(radcontent STATIC ${RADCONTENT_SOURCES})
target_include_directories(radcontent
	PUBLIC "inc/"
	PRIVATE "src/"
)
target_link_libraries(radcontent
	radcore
	radmath
	pddi
)
