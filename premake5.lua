include "export-compile-commands.lua"

-- Automating finding and setting up framework directories
local framework_paths = os.matchdirs("third_party/**.framework")
local framework_rpaths = {}

for _, fw_path in ipairs(framework_paths) do
   local parent_dir = path.getdirectory(fw_path)

   local rpath = "-rpath @executable_path/" .. parent_dir
   table.insert(framework_rpaths, rpath)
end

-- Automating finding library folders and adding to post buildcommands for windows
local lib_dirs = os.matchdirs("third_party/*/lib")
local dll_copy_commands = {}

for _, dir in ipairs(lib_dirs) do
   table.insert(dll_copy_commands, "{COPY} \"" .. dir .. "/*.dll\" \"%{cfg.targetdir}\"")
end

workspace "game_engine"
   configurations { "Release" }
   -- architecture "x64"
   startproject "game_engine"

   filter "action:gmake*"
      toolset "clang"

   filter "action:vs*"
      architecture "x64"
      
   filter {}

-- project "Lua"
--    kind "StaticLib"
--    language "C"
   
--    targetdir "bin/%{cfg.buildcfg}/Lua"
   
--    files { 
--       "third_party/Lua/*.c", 
--       "third_party/Lua/*.h",
--       "third_party/Lua/*.hpp"
--    }

--    excludes {
--       "third_party/Lua/lua.c",
--       "third_party/Lua/luac.c"
--    }

--    filter "action:vs*"
--       defines { "_CRT_SECURE_NO_WARNINGS" }

--    filter "action:gmake*"
--       defines { "LUA_USE_LINUX" }
--       buildoptions { "-O2" }

project "box2d"
   kind "StaticLib"
   language "C++"
   cppdialect "C++17"

   targetdir "bin/%{cfg.buildcfg}/box2d"
   objdir "bin-int/%{cfg.buildcfg}/box2d"

   includedirs {
      "third_party/box2d/include",
      "third_party/box2d/src"
   }

   files { 
      "third_party/box2d/src/**.cpp",
      "third_party/box2d/src/**.h",
      "third_party/box2d/include/**.h"
   }

   filter "configurations:Release"
      optimize "Speed"
   filter {}

project "game_engine"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"

   files { 
      "src/*.cpp",
      "third_party/Lua/*.c", 
      "third_party/Lua/*.h",
      "third_party/Lua/*.hpp",
      "third_party/box2d/src/**.cpp",
      "third_party/box2d/src/**.h",
      "third_party/box2d/include/**.h"
   }
   
   excludes {
      "build/**",
      "bin/**",
      "CMakeFiles/**",
      "**/CMakeCXXCompilerId.cpp",
      "third_party/Lua/lua.c",
      "third_party/Lua/luac.c"
   }
   
   includedirs { 
      "include",
      "third_party/box2d/include",
      "third_party/box2d/src"
   }

   -- ================= WINDOWS (Visual Studio) =================
   filter "action:vs*"
      architecture "x64"
      filename "game_engine"
      targetname "game_engine_win"
      targetdir "x64/Release"
      
      optimize "Speed"
      defines { 
         "_CRT_SECURE_NO_WARNINGS", 
         "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING" 
      }

      libdirs { lib_dirs }
      links { 
         "SDL2main",
         "SDL2", 
         "SDL2_image",
         "SDL2_mixer",
         "SDL2_ttf",
      }

      postbuildcommands(dll_copy_commands)

   -- ================= MAC OS (Xcode) =================
   filter "action:xcode4"
      externalincludedirs {
         "third_party", 
         "third_party/SDL2", 
         "third_party/Lua",
         "third_party/LuaBridge",
         "third_party/box2d/include"
      }

      filename "game_engine"
      targetname "game_engine_osx"
      targetdir "."

      optimize "Speed"
      
      xcodebuildsettings {
         ["SDKROOT"] = "macosx",
         ["GCC_OPTIMIZATION_LEVEL"] = "3"
      }

      frameworkdirs { os.matchdirs("third_party/*/lib") }
      linkoptions { framework_rpaths }
      links {
         "SDL2.framework", 
         "SDL2_image.framework",
         "SDL2_mixer.framework",
         "SDL2_ttf.framework"
      }

   filter "not action:xcode4"
      includedirs {
         "third_party", 
         "third_party/SDL2", 
         "third_party/Lua",
         "third_party/LuaBridge"
      }

   -- ================= LINUX (Makefile) =================
   filter "action:gmake*"
      filename "Makefile"
      targetname "game_engine_linux"
      targetdir "."
      toolset "clang"

      optimize "Speed"
      buildoptions { "-O3", "-Wall", "-Wextra" }

      defines { "LUA_USE_LINUX" }

      libdirs { "third_party/SDL2_ttf/lib" }
      linkoptions { "-Wl,-rpath,'$$ORIGIN/third_party/SDL2_ttf/lib'" }

      links { 
          "SDL2_image", 
          "SDL2_mixer", 
          "SDL2_ttf", 
          "SDL2" 
      }

   filter {}


require('gmake')

premake.override(premake.action.get("gmake"), "onWorkspace", function(base, wks)
    base(wks) 
    
    local f = io.open("Makefile", "a")
    if f then
        f:write("\n\nCXX = clang++\n")
        f:write("CXXFLAGS += -O3\n")
        f:close()
        print(">> Autograder flags successfully appended to Makefile.")
    end
end)