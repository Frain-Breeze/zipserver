
target_include_directories("zipserver" PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include/"
	"${CMAKE_CURRENT_SOURCE_DIR}/bit7z/include/"
)
add_definitions(-DBIT7Z_AUTO_FORMAT)
add_subdirectory("bit7z")
