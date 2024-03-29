cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)

project(lpvpn)

message(STATUS "Platform: ${PLATFORM}")
message(STATUS "Generator Platform: ${CMAKE_GENERATOR_PLATFORM}")

set(STEAM_APP_ID 1122990)

set(LPVPN_VERSION 0.1.3)
set(LPVPN_WIN32_VERSION 0,1,3,0)

if (NOT DEFINED GIT_EXECUTABLE)
	set(GIT_EXECUTABLE "git")
endif()

execute_process(COMMAND
	"${GIT_EXECUTABLE}" describe --match=NeVeRmAtCh --always --dirty
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE LPVPN_GIT_VERSION
	ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

include(FetchContent)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tmp")

if (WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_UNICODE)
	add_definitions(-DUNICODE)
	add_definitions(-DLPVPN_VERSION="${LPVPN_VERSION}")
	add_definitions(-DLPVPN_WIN32_VERSION=${LPVPN_WIN32_VERSION})
	add_definitions(-DLPVPN_GIT_VERSION="${LPVPN_GIT_VERSION}")
	set(CMAKE_SYSTEM_VERSION 6.1)
endif()

include(cmake/Steamworks.cmake)
include(cmake/wintun.cmake)
include(cmake/CMakeRC.cmake)


file(GLOB_RECURSE RESOURCES RELATIVE ${CMAKE_SOURCE_DIR} "resources/cmrc/*")
cmrc_add_resource_library(lpvpn-resources ALIAS lpvpn::rc NAMESPACE lpvpn)
cmrc_add_resources(lpvpn-resources WHENCE resources/cmrc ${RESOURCES})

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")
add_executable(lpvpn ${SOURCES})
set_property(TARGET lpvpn PROPERTY CXX_STANDARD 20)

target_compile_definitions(
	lpvpn PUBLIC
	STEAM_APP_ID=${STEAM_APP_ID}
)

target_link_libraries(lpvpn PRIVATE lpvpn::rc Steamworks)

if (WIN32)
	set_target_properties(lpvpn PROPERTIES
		LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:WINDOWS"
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		VS_DPI_AWARE "PerMonitor"
		PROJECT_LABEL "PartyLAN"
	)
	target_sources(lpvpn PRIVATE "${CMAKE_SOURCE_DIR}/resources/windows/resource.rc")
	# add to include path
	target_include_directories(lpvpn PRIVATE "${CMAKE_SOURCE_DIR}/resources/windows")
	target_link_libraries(lpvpn PRIVATE Wintun)
endif()

# create steam_appid.txt if compiling debug
file(
	WRITE
	"${CMAKE_SOURCE_DIR}/resources/steam_appid.txt"
	"${STEAM_APP_ID}"
)

add_custom_command(
	TARGET lpvpn POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy
			"${Steamworks_REDISTRIBUTABLE}"
			"${CMAKE_SOURCE_DIR}/LICENSE"
			"$<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/resources/steam_appid.txt,>"
			"$<TARGET_FILE_DIR:lpvpn>"
)

if (WIN32)
	# copy wintun.dll
	add_custom_command(
		TARGET lpvpn POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
				"${Wintun_REDISTRIBUTABLE}"
				"$<TARGET_FILE_DIR:lpvpn>"
	)
endif()
