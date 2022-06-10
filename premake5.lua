-- premake5.lua
workspace "P4ALL_OCR-TABLES"
    architecture "x86"
    configurations { 
        "Debug", 
        "Release" 
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "P4ALL_OCR-TABLES"
    location "P4ALL_OCR-TABLES"
    kind "ConsoleApp"
    language "C++"
    targetdir ("bin/" .. outputdir .. "%{prj.name}") 
    objdir ("bin-int/" .. outputdir .. "%{prj.name}") 

    files { 
        "%{prj.name}/src/**.h", 
        "%{prj.name}/src/**.cpp" 
    }

    includedirs {
        "%{prj.name}/vendor/OpenCV/build/include",
        "%{prj.name}/vendor/MuPDF/include",
        "%{prj.name}/vendor/tesseract/include",
        "%{prj.name}/vendor/leptonica/leptonica/include",
        "%{prj.name}/src"
    }

    libdirs {
        "%{prj.name}/vendor/OpenCV/build/x86/vc14/lib",
        "%{prj.name}/vendor/MuPDF/platform/win32/Debug",
        "%{prj.name}/vendor/tesseract/lib",
        "%{prj.name}/vendor/leptonica/leptonica/lib"
    }

    links {
        "libmupdf",
        "libthirdparty",
        "opencv_core2413",
        "opencv_highgui2413",
        "opencv_imgproc2413",
        "tesseract41",
        "leptonica-1.82.0"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        
    postbuildcommands {
        ("{COPY} %{prj.name}/vendor/OpenCV/build/x86/vc14/bin/ %{cfg.buildtarget.relpath")
    }