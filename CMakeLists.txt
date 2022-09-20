cmake_minimum_required(VERSION 3.10)

project("zipserve")

if(WIN32)
    message("building for windows - good luck")
    link_libraries(-static-libgcc -static-libstdc++ -static)
	add_compile_definitions(ON_WINDOWS)
	
	set(LIBLZMA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/liblzma/include/")
	set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/zlib/include/zlib/")
	
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(LIBLZMA_LIBRARY "${CMAKE_SOURCE_DIR}/lib/liblzma/lib/x64/liblzma.lib")
		set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib/lib/zlib_x64/zlibstatic.lib")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(LIBLZMA_LIBRARY "${CMAKE_SOURCE_DIR}/lib/liblzma/lib/x86/liblzma.lib")
		set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib/lib/zlib_x86/zlibstatic.lib")
	endif()
else()

endif()

find_package(ZLIB REQUIRED)
find_package(LibLZMA REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()
if (LIBLZMA_FOUND)
    include_directories(${LZMA_INCLUDE_DIRS})
endif()

#disable building of stuff we don't need
set(ENABLE_CPIO OFF)
set(ENABLE_CAT  OFF)
set(ENABLE_TAR  OFF)
#set(ENABLE_LZMA OFF)
set(ENABLE_OPENSSL OFF)
#set(ENABLE_ZLIB OFF) #will cry on debug if turned off
set(ENABLE_BZip2 OFF)
set(ENABLE_LIBB2 OFF)
set(ENABLE_LZ4 OFF)
set(ENABLE_ZSTD OFF)
#set(ENABLE_LIBXML2 OFF) #needed for "xar"?
set(ENABLE_TEST OFF)
set(ENABLE_INSTALL OFF)
set(ENABLE_WERROR OFF)
add_subdirectory("lib/libarchive/")

add_subdirectory("zipserver")
