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

if ("${BINARY_PLATFORM}" STREQUAL "x64")
	set (Wintun_REDISTRIBUTABLE "${wintun_SOURCE_DIR}/bin/amd64/wintun.dll")
	set (Wintun_REDISTRIBUTABLE_DIR "${wintun_SOURCE_DIR}/bin/amd64")
elseif ("${BINARY_PLATFORM}" STREQUAL "win32")
	set (Wintun_REDISTRIBUTABLE "${wintun_SOURCE_DIR}/bin/x86/wintun.dll")
	set (Wintun_REDISTRIBUTABLE_DIR "${wintun_SOURCE_DIR}/bin/x86")
endif()

execute_process(COMMAND python "${CMAKE_SOURCE_DIR}/scripts/defgen.py" "${Wintun_REDISTRIBUTABLE_DIR}/wintun.dll" -o wintun.def WORKING_DIRECTORY "${wintun_SOURCE_DIR}")
execute_process(COMMAND lib /machine:x64 /def:../../wintun.def /out:wintun.lib WORKING_DIRECTORY "${Wintun_REDISTRIBUTABLE_DIR}")
execute_process(COMMAND python "${CMAKE_SOURCE_DIR}/scripts/wintunheadergen.py" wintun.def -o include/wintunimp.h WORKING_DIRECTORY "${wintun_SOURCE_DIR}")
set (Wintun_IMPLIB "${Wintun_REDISTRIBUTABLE_DIR}/wintun.lib")

add_library(Wintun SHARED IMPORTED)
set_target_properties(Wintun PROPERTIES IMPORTED_LOCATION "${Wintun_REDISTRIBUTABLE}")
set_target_properties(Wintun PROPERTIES IMPORTED_IMPLIB "${Wintun_IMPLIB}")
target_include_directories(Wintun INTERFACE "${Wintun_INCLUDE_DIR}")
