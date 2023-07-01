include(FetchContent)

if (DEFINED ENV{STEAMWORKS_SDK_URL})
	set (STEAMWORKS_SDK_URL "$ENV{STEAMWORKS_SDK_URL}")
endif()

if (NOT DEFINED STEAMWORKS_SDK_URL)
	set(STEAMWORKS_SDK_URL "https://partner.steamgames.com/downloads/steamworks_sdk_156.zip")
endif()

FetchContent_Declare(
	Steamworks
	URL "${STEAMWORKS_SDK_URL}"
)

FetchContent_GetProperties(Steamworks)
if (NOT Steamworks_POPULATED)
	FetchContent_Populate(Steamworks)
endif()

set (Steamworks_INCLUDE_DIR "${steamworks_SOURCE_DIR}/public/steam")

# win32

if (WIN32)
	if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
		set (Steamworks_REDISTRIBUTABLE "${steamworks_SOURCE_DIR}/redistributable_bin/win64/steam_api64.dll")
		set (Steamworks_REDISTRIBUTABLE_DIR "${steamworks_SOURCE_DIR}/redistributable_bin/win64")
		set (Steamworks_IMPLIB "${steamworks_SOURCE_DIR}/redistributable_bin/win64/steam_api64.lib")
	elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
		set (Steamworks_REDISTRIBUTABLE "${steamworks_SOURCE_DIR}/redistributable_bin/steam_api.dll")
		set (Steamworks_REDISTRIBUTABLE_DIR "${steamworks_SOURCE_DIR}/redistributable_bin")
		set (Steamworks_IMPLIB "${steamworks_SOURCE_DIR}/redistributable_bin/steam_api.lib")
	else()
		message(FATAL_ERROR "Unsupported platform: ${CMAKE_GENERATOR_PLATFORM}")
	endif()
	add_library(Steamworks SHARED IMPORTED)
	set_target_properties(Steamworks PROPERTIES IMPORTED_LOCATION "${Steamworks_REDISTRIBUTABLE}")
	set_target_properties(Steamworks PROPERTIES IMPORTED_IMPLIB "${Steamworks_IMPLIB}")
	target_include_directories(Steamworks INTERFACE "${Steamworks_INCLUDE_DIR}")
else()
	message(FATAL_ERROR "Unsupported platform: ${CMAKE_GENERATOR_PLATFORM}")
endif()
