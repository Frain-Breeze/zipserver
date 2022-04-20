project("zipserver")

set(CMAKE_CXX_STANDARD 17)

add_executable("zipserver"
	"src/main.cpp"
	"src/ftp_server.cpp"
)
target_link_libraries("zipserver"
	#$<$<BOOL:${WIN32}>:archive>
	archive_static
)
target_include_directories("zipserver" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_include_directories("zipserver" PRIVATE
	"${CMAKE_SOURCE_DIR}/lib/asio/asio/include/"
	"${CMAKE_SOURCE_DIR}/lib/libarchive/libarchive/"

)
#add_custom_command(TARGET "zipserver" POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:archive> $<TARGET_PROPERTY:zipserver,BINARY_DIR>/${CMAKE_BUILD_TYPE}/archive.dll
#)
