set(RADMUSIC_SOURCES
	"src/memory/memory.cpp"
	"src/ods/ods_codegen.cpp"
	"src/ods/ods.cpp"
	"src/ods/ods_memory.cpp"
	"src/ods/ods_parser.cpp"
	"src/ods/ods_util.cpp"
	"src/pch/pch.cpp"
	"src/radmusic/framework/framework.cpp"
	"src/radmusic/music/music_composition.cpp"
	"src/radmusic/music/music_engine.cpp"
	"src/radmusic/music/music_engine_state_fade.cpp"
	"src/radmusic/music/music_engine_state_steady.cpp"
	"src/radmusic/music/music_engine_state_stitch.cpp"
	"src/radmusic/music/music_engine_state_stopped.cpp"
	"src/radmusic/music/music_performance.cpp"
	"src/radmusic/resource/resource_manager.cpp"
	"src/radmusic/schema/schema_util.cpp"
	"src/radmusic/sequence/sequence_player.cpp"
	"src/radmusic/sequence/sequence_region.cpp"
	"src/radmusic/sequence/sequence_stream_graph.cpp"
)

add_library(radmusic STATIC ${RADMUSIC_SOURCES})
target_include_directories(radmusic
	PUBLIC "inc/"
	PRIVATE "src/pch/"
	PRIVATE "src/"
)
target_link_libraries(radmusic
	radcore
	radcontent
	radsound
)
