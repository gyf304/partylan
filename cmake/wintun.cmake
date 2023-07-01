include(FetchContent)

FetchContent_Declare(
    Wintun
    URL https://www.wintun.net/builds/wintun-0.14.1.zip
)

FetchContent_GetProperties(Wintun)
if (NOT wintun_POPULATED)
	FetchContent_Populate(Wintun)
endif()

set (Wintun_INCLUDE_DIR "${wintun_SOURCE_DIR}/include")
# create def file for wintun
set (Wintun_DEF_FILE "${wintun_SOURCE_DIR}/wintun.def")

if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
	set (Wintun_REDISTRIBUTABLE "${wintun_SOURCE_DIR}/bin/amd64/wintun.dll")
	set (Wintun_REDISTRIBUTABLE_DIR "${wintun_SOURCE_DIR}/bin/amd64")
elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
	set (Wintun_REDISTRIBUTABLE "${wintun_SOURCE_DIR}/bin/x86/wintun.dll")
	set (Wintun_REDISTRIBUTABLE_DIR "${wintun_SOURCE_DIR}/bin/x86")
else()
	message(FATAL_ERROR "Unsupported platform: ${CMAKE_GENERATOR_PLATFORM}")
endif()

add_library(Wintun INTERFACE)
set_target_properties(Wintun PROPERTIES IMPORTED_LOCATION "${Wintun_REDISTRIBUTABLE}")
target_include_directories(Wintun INTERFACE "${Wintun_INCLUDE_DIR}")
