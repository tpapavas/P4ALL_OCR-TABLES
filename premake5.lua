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
        "C:/OpenCV–2.4.13.6/build/x86/vc14/lib",
        dependencesdir .. "/mupdf-1.19.0/platform/win32/Debug",
        dependencesdir .. "/vcpkg/packages/tesseract_x86-windows/lib",
        dependencesdir .. "/vcpkg/packages/leptonica_x86-windows/lib"
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