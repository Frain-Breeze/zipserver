project("zipserver")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_DEBUG_POSTFIX "_d")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "Build type not specified: using Release by default.")
endif()

# per-architecture options
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Architecture: x86")
    set(ARCH_POSTFIX 32)
else()
    message(STATUS "Architecture: x64")
    set(ARCH_POSTFIX 64)
    add_definitions(-DWIN64)
endif()

set(TARGET_NAME "zipserver_${ARCH_POSTFIX}")

set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})

add_executable(${TARGET_NAME}
	"src/main.cpp"
	"src/ftp_server.cpp"
)
target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_include_directories(${TARGET_NAME} PRIVATE
	"${CMAKE_SOURCE_DIR}/lib/asio/asio/include/"
	"${CMAKE_SOURCE_DIR}/lib/bit7z/include/"
)

add_definitions(-DBIT7Z_AUTO_FORMAT)
target_link_libraries(${TARGET_NAME} bit7z${ARCH_POSTFIX})