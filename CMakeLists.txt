cmake_minimum_required(VERSION 4.0)

project(game)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

set(CMAKE_CXX_STANDARD 23)

add_executable(game
	src/application.hpp
	src/application.cpp
	src/game.hpp
	src/game.cpp
	src/audio.hpp
	src/audio.cpp
	src/input.hpp
	src/input.cpp
	src/time.hpp
	src/ui.hpp
	src/ui.cpp
	src/text.hpp
	src/asset.hpp
	src/asset.cpp
	src/draw.hpp
	src/draw.cpp
	src/log.hpp
	src/log.cpp
	src/common.hpp
	src/common.cpp
	src/math_util.hpp
	src/math_util.cpp
	src/script.hpp
	src/script.cpp
	src/vehicle.hpp
	src/vehicle.cpp
	src/map.hpp
	src/map.cpp
	src/space.hpp
	src/space.cpp
	src/editor.hpp
	src/editor.cpp
	src/template.hpp
	src/main.cpp

	src/external/open_simplex.hpp
	src/external/open_simplex.cpp
)

add_library(language STATIC
	src/language/token.hpp
	src/language/expr.hpp
	src/language/statement.hpp
	src/language/lang.hpp
	src/language/lang_common.hpp
	src/language/function.hpp
	src/language/bytecode.hpp
	src/language/language.cpp
	src/language/bytecode.cpp
	src/language/function.cpp
)

add_library(lua STATIC
	src/lua/src/lapi.c
	src/lua/src/lapi.h
	src/lua/src/lauxlib.c
	src/lua/src/lauxlib.h
	src/lua/src/lbaselib.c
	src/lua/src/lcode.c
	src/lua/src/lcode.h
	src/lua/src/lcorolib.c
	src/lua/src/lctype.c
	src/lua/src/lctype.h
	src/lua/src/ldblib.c
	src/lua/src/ldebug.c
	src/lua/src/ldebug.h
	src/lua/src/ldo.c
	src/lua/src/ldo.h
	src/lua/src/ldump.c
	src/lua/src/lfunc.c
	src/lua/src/lfunc.h
	src/lua/src/lgc.c
	src/lua/src/lgc.h
	src/lua/src/linit.c
	src/lua/src/liolib.c
	src/lua/src/ljumptab.h
	src/lua/src/llex.c
	src/lua/src/llex.h
	src/lua/src/llimits.h
	src/lua/src/lmathlib.c
	src/lua/src/lmem.c
	src/lua/src/lmem.h
	src/lua/src/loadlib.c
	src/lua/src/lobject.c
	src/lua/src/lobject.h
	src/lua/src/lopcodes.c
	src/lua/src/lopcodes.h
	src/lua/src/lopnames.h
	src/lua/src/loslib.c
	src/lua/src/lparser.c
	src/lua/src/lparser.h
	src/lua/src/lprefix.h
	src/lua/src/lstate.c
	src/lua/src/lstate.h
	src/lua/src/lstring.c
	src/lua/src/lstring.h
	src/lua/src/lstrlib.c
	src/lua/src/ltable.c
	src/lua/src/ltable.h
	src/lua/src/ltablib.c
	src/lua/src/ltm.c
	src/lua/src/ltm.h
	src/lua/src/lua.c
	src/lua/src/lua.h
	src/lua/src/lua.hpp
	src/lua/src/luaconf.h
	src/lua/src/lualib.h
	src/lua/src/lundump.c
	src/lua/src/lundump.h
	src/lua/src/lutf8lib.c
	src/lua/src/lvm.c
	src/lua/src/lvm.h
	src/lua/src/lzio.c
	src/lua/src/lzio.h
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)

target_link_libraries(game PRIVATE language)
target_link_libraries(game PRIVATE lua)

target_include_directories(game PRIVATE src/)
target_include_directories(language PRIVATE src/)
target_include_directories(game PRIVATE src/lua/src)

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# turn unused image formats off
set(SDLIMAGE_ANI  OFF)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP  OFF)
set(SDLIMAGE_GIF  OFF)
set(SDLIMAGE_JPG  OFF)
set(SDLIMAGE_JXL  OFF)
set(SDLIMAGE_LBM  OFF)
set(SDLIMAGE_PCX  OFF)
set(SDLIMAGE_PNM  OFF)
set(SDLIMAGE_QOI  OFF)
set(SDLIMAGE_TGA  OFF)
set(SDLIMAGE_TIF  OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_XCF  OFF)
set(SDLIMAGE_XPM  OFF)
set(SDLIMAGE_XV   OFF)
# png
set(SDLIMAGE_PNG ON)
set(SDLIMAGE_SVG  ON)
add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)

add_subdirectory(vendor/SDL_mixer EXCLUDE_FROM_ALL)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)

target_include_directories(game PRIVATE vendor/SDL/include/)
target_include_directories(game PRIVATE vendor/SDL_image/include/)
target_include_directories(game PRIVATE vendor/SDL_mixer/include)
target_include_directories(game PRIVATE vendor/SDL_ttf/include/)

target_link_libraries(
	game
	PRIVATE
	SDL3::SDL3
	SDL3_image::SDL3_image
	SDL3_mixer::SDL3_mixer
	SDL3_ttf::SDL3_ttf
)
