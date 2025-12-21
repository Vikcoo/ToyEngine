@echo off
REM Shader 编译脚本
REM 使用 Vulkan SDK 的 glslc 编译器

set VULKAN_SDK_PATH=%VULKAN_SDK%
if "%VULKAN_SDK_PATH%"=="" (
    echo Error: VULKAN_SDK environment variable is not set!
    echo Please install Vulkan SDK and set VULKAN_SDK environment variable.
    pause
    exit /b 1
)

set GLSLC=%VULKAN_SDK_PATH%\Bin\glslc.exe
if not exist "%GLSLC%" (
    echo Error: glslc.exe not found at: %GLSLC%
    pause
    exit /b 1
)

echo Compiling shaders...
echo.

REM 编译顶点着色器
echo Compiling triangle.vert...
"%GLSLC%" "Content\Shaders\Common\triangle.vert" -o "Content\Shaders\Common\triangle.vert.spv"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile triangle.vert
    pause
    exit /b 1
)
echo triangle.vert compiled successfully!
echo.

REM 编译片段着色器
echo Compiling triangle.frag...
"%GLSLC%" "Content\Shaders\Common\triangle.frag" -o "Content\Shaders\Common\triangle.frag.spv"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile triangle.frag
    pause
    exit /b 1
)
echo triangle.frag compiled successfully!
echo.

echo All shaders compiled successfully!
pause

