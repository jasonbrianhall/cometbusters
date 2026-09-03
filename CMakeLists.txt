cmake_minimum_required(VERSION 3.21)
project(cometbuster LANGUAGES CXX C)

set(PACKAGE_VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Steam is Linux-only and must never be enabled for the Windows/MS Store build
option(STEAM_SUPPORT "Enable Steamworks integration (Linux only)" OFF)
if(STEAM_SUPPORT AND WIN32)
  message(FATAL_ERROR "STEAM_SUPPORT must not be enabled for Windows/MS Store builds")
endif()

# ----------------------------------------------------------------------------
# Required generated header (font data) - same check as `make check-monospace`
# ----------------------------------------------------------------------------
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Monospace.h")
  message(FATAL_ERROR
    "Monospace.h not found. Generate it first, e.g.:\n"
    "  python3 ttf_to_base64_header.py <font.ttf> Monospace.h")
endif()

# ----------------------------------------------------------------------------
# Dependencies
#   Windows/MSVC: resolved via vcpkg manifest mode (sdl2, sdl2-mixer, freetype, glew)
#   Linux: SDL2/SDL2_mixer have no system CMake config on most distros (Fedora
#   included), so those two are resolved via pkg-config instead. Freetype,
#   GLEW and OpenGL have standard CMake find-modules that work either way.
# ----------------------------------------------------------------------------
find_package(Freetype REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

if(WIN32)
  find_package(SDL2 CONFIG REQUIRED)
  find_package(SDL2_mixer CONFIG REQUIRED)
  set(SDL2_TARGET SDL2::SDL2)
  set(SDL2MAIN_TARGET SDL2::SDL2main)
  set(SDL2_MIXER_TARGET SDL2_mixer::SDL2_mixer)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
  set(SDL2_TARGET PkgConfig::SDL2)
  set(SDL2MAIN_TARGET "")
  set(SDL2_MIXER_TARGET PkgConfig::SDL2_MIXER)
endif()

if(STEAM_SUPPORT)
  set(STEAM_SDK "$ENV{HOME}/software_build/steamworks/sdk" CACHE PATH "Path to Steamworks SDK")
  if(NOT EXISTS "${STEAM_SDK}/public/steam/steam_api.h")
    message(FATAL_ERROR "Steamworks SDK not found at ${STEAM_SDK} (set -DSTEAM_SDK=...)")
  endif()
  add_library(steam_api SHARED IMPORTED)
  set_target_properties(steam_api PROPERTIES
    IMPORTED_LOCATION "${STEAM_SDK}/redistributable_bin/linux64/libsteam_api.so"
    INTERFACE_INCLUDE_DIRECTORIES "${STEAM_SDK}/public"
  )
endif()

# ----------------------------------------------------------------------------
# Sources (matches SOURCES_CPP_COMMON / SOURCES_C in Makefile.gl)
# ----------------------------------------------------------------------------
set(SOURCES_CPP
  comet_main_gl.cpp
  comet_main_gl_handle_events.cpp
  wad.cpp
  audio_wad.cpp
  cometbuster_init.cpp
  cometbuster_physics.cpp
  cometbuster_collision.cpp
  cometbuster_boss.cpp
  cometbuster_starboss.cpp
  cometbuster_render_gl.cpp
  cometbuster_util.cpp
  cometbuster_splashscreen.cpp
  joystick.cpp
  cometbuster_bombs.cpp
  cometbuster_bossexplosion.cpp
  comet_highscores.cpp
  comet_preferences.cpp
  cometbuster_spawn.cpp
  comet_main_gl_menu.cpp
  comet_haptics.cpp
  comet_save.cpp
  cometbuster_render_gl_font.cpp
)

set(SOURCES_C
  miniz.c
  miniz_tdef.c
  miniz_tinfl.c
  miniz_zip.c
)

# WIN32 marks the exe as a GUI subsystem app on Windows (equivalent to -mwindows);
# ignored on other platforms.
add_executable(cometbuster WIN32 ${SOURCES_CPP} ${SOURCES_C})

target_compile_definitions(cometbuster PRIVATE
  GLSave
  ExternalSound
  VERSION="${PACKAGE_VERSION}"
  $<$<PLATFORM_ID:Windows>:WIN32>
  $<$<PLATFORM_ID:Windows>:_WIN32>
  $<$<PLATFORM_ID:Linux>:LINUX>
  $<$<CONFIG:Debug>:DEBUG>
  $<$<BOOL:${STEAM_SUPPORT}>:STEAM_ENABLED>
)

target_link_libraries(cometbuster PRIVATE
  ${SDL2_TARGET}
  ${SDL2MAIN_TARGET}
  ${SDL2_MIXER_TARGET}
  Freetype::Freetype
  GLEW::GLEW
  OpenGL::GL
  $<$<PLATFORM_ID:Windows>:winmm>
  $<$<BOOL:${STEAM_SUPPORT}>:steam_api>
)

if(MSVC)
  # GCC's -fpermissive relaxes strict conformance; MSVC's default (no /permissive-)
  # is already the closest equivalent. Some source may still need small fixes
  # when first compiled under MSVC - this isn't a drop-in guarantee.
  target_compile_options(cometbuster PRIVATE /W3 /EHsc /MP)
  target_compile_options(cometbuster PRIVATE $<$<CONFIG:Release>:/O2 /GL>)
  target_link_options(cometbuster PRIVATE $<$<CONFIG:Release>:/LTCG>)
else()
  target_compile_options(cometbuster PRIVATE -Wall -Wextra -fpermissive)
endif()

# ----------------------------------------------------------------------------
# WAD asset build (equivalent to cometbuster-create-wad)
# ----------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_command(
  OUTPUT "${CMAKE_SOURCE_DIR}/cometbuster.wad"
  COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/createwad.py"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Building cometbuster.wad"
  VERBATIM
)
add_custom_target(cometbuster_wad DEPENDS "${CMAKE_SOURCE_DIR}/cometbuster.wad")
add_dependencies(cometbuster cometbuster_wad)

add_custom_command(TARGET cometbuster POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/cometbuster.wad" "$<TARGET_FILE_DIR:cometbuster>"
  COMMENT "Copying cometbuster.wad next to executable"
)
