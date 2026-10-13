set(SCROOBY_SOURCES
	"src/FeApp.cpp"
	"src/FeBoundedDrawable.cpp"
	"src/FeChunkHandler.cpp"
	"src/FeDrawable.cpp"
	"src/FeEntity.cpp"
	"src/FeGroup.cpp"
	"src/FeLanguage.cpp"
	"src/FeLayer.cpp"
	"src/FeLoaders.cpp"
	"src/FeMovie.cpp"
	"src/FeOwner.cpp"
	"src/FePage.cpp"
	"src/FeParent.cpp"
	"src/FePolygon.cpp"
	"src/FeProject.cpp"
	"src/FePure3dObject.cpp"
	"src/FeScreen.cpp"
	"src/FeSprite.cpp"
	"src/FeTextBible.cpp"
	"src/FeText.cpp"
	"src/FeTextStyle.cpp"
	"src/ResourceManager/FeResourceManager.cpp"
	"src/scrooby/App.cpp"
	"src/strings/pcstring.cpp"
	"src/strings/stricmp.cpp"
	"src/strings/UnicodeString.cpp"
	"src/tLinearTable.cpp"
	"src/utility/AsyncFileLoader.cpp"
	"src/utility/BufferReader.cpp"
	"src/utility/debugMessages.cpp"
	"src/utility/EnumConversion.cpp"
	"src/xml/XMLParser.cpp"
	"src/xml/XMLTree.cpp"
)

add_library(scrooby STATIC ${SCROOBY_SOURCES})
target_include_directories(scrooby
	PUBLIC "inc/"
	PUBLIC "src/"
)
target_link_libraries(scrooby
	p3d
)
