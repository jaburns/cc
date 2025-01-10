#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "../vendor/minivorbis.h"
#include "../vendor/stb_image.h"

#if EDITOR
#include "../vendor/imgui/imgui_impl_sdl2.h"
#include "../vendor/imgui/imgui_impl_vulkan.h"
#endif

#include "../base/inc.hh"

#include "audio.hh"
#include "gfx.hh"
#include "pipeline.hh"

#if EDITOR
#define app_dll_export extern "C"
#else
#define app_dll_export static
#endif
