set(RADSCRIPT_SOURCES
	"src/factory/radfactory.cpp"
	"src/namespace/namespace.cpp"
	"src/pch/pch.cpp"
	"src/script/remotescript.cpp"
	"src/script/script.cpp"
	"src/typeinfo/TypeInfoEnum.cpp"
	"src/typeinfo/TypeInfoInstance.cpp"
	"src/typeinfo/TypeInfoInterface.cpp"
	"src/typeinfo/TypeInfoIntLiteral.cpp"
	"src/typeinfo/TypeInfoLoader.cpp"
	"src/typeinfo/TypeInfoMethod.cpp"
	"src/typeinfo/TypeInfoParam.cpp"
	"src/typeinfo/TypeInfoSystem.cpp"
	"src/typeinfoutil/typeinfodistributor.cpp"
	"src/typeinfoutil/typeinfoutil.cpp"
	"src/typeinfo/win32/win32typeinfovfcall.cpp"
)

add_library(radscript STATIC ${RADSCRIPT_SOURCES})
target_include_directories(radscript
	PUBLIC "inc/"
	PRIVATE "src/pch/"
)
target_link_libraries(radscript
	radcore
)
