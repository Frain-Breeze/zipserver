cmake_minimum_required(VERSION 3.12)

project("zipserve")

set(CMAKE_CXX_STANDARD 17)

add_executable("zipserver"
	"src/main.cpp"
)

target_include_directories("zipserver" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
add_subdirectory("lib")
target_link_libraries("zipserver" "${CMAKE_CURRENT_SOURCE_DIR}/lib/bit7z/bin/x64/bit7z64.lib")