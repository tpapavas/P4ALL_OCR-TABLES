-- premake5.lua
workspace "P4ALL_OCR-TABLES"
    architecture "x86"
    configurations { 
        "Debug", 
        "Release" 
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
dependencesdir ="C:/dev"

project "P4ALL_OCR-TABLES"
    location "P4ALL_OCR-TABLES"
    kind "ConsoleApp"
    language "C++"
    targetdir ("bin/" .. outputdir .. "%{prj.name}") 
    objdir ("bin_int/" .. outputdir .. "%{prj.name}") 

    files { 
        "%{prj.name}/src/**.h", 
        "%{prj.name}/src/**.cpp" 
    }

    includedirs {
        dependencesdir .. "/mupdf-1.19.0/include",
        "%{prj.name}/src"
    }

    libdirs {
        dependencesdir .. "/mupdf-1.19.0/platform/win32/Debug",
    }

    links {
        "libmupdf",
        "libthirdparty",
        "opencv_core",
        "opencv_highgui",
        "opencv_imgproc",
        "tesseract41",
        "leptonica-1.82.0"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        
    -- postbuildcommands {
        -- ("{COPY} %{prj.name}/vendor/OpenCV/build/x86/vc14/bin/ %{cfg.buildtarget.relpath")
    -- }