@echo off
setlocal enabledelayedexpansion

:: Set directories (update these paths if needed)
set SHADER_SOURCE_DIR=%CD%\src\Shaders
set SHADER_OUTPUT_DIR=%CD%\Image\Resources\Shaders

echo Deleting the output directory to clear the cache...
rmdir /s /q "%SHADER_OUTPUT_DIR%"

:: Make sure the output directory exists
if not exist "%SHADER_OUTPUT_DIR%" (
    echo Creating output directory: "%SHADER_OUTPUT_DIR%"
    mkdir "%SHADER_OUTPUT_DIR%"
)

:: Path to glslangValidator (update if needed)
set GLSLANG_VALIDATOR="C:\VulkanSDK\1.4.313.2\Bin\glslangValidator.exe"

:: Check if glslangValidator exists
if not exist %GLSLANG_VALIDATOR% (
    echo Error: glslangValidator not found! Please check the Vulkan SDK path.
    exit /b 1
)

:: Loop through all .vert and .frag files in the source directory
for /r "%SHADER_SOURCE_DIR%" %%f in (*.vert *.frag *.comp) do (
    :: Get the shader filename without extension
    set "SHADER_FILE=%%f"
    set "SHADER_NAME=%%~nf"
    
    :: Define the output shader path
    set "OUTPUT_SHADER=%SHADER_OUTPUT_DIR%\!SHADER_NAME!.spv"

    echo Compiling shader: !SHADER_FILE! to !OUTPUT_SHADER!
    
    :: Run glslangValidator to compile the shader
    "%GLSLANG_VALIDATOR%" -V "!SHADER_FILE!" -o "!OUTPUT_SHADER!" -g
    if %errorlevel% neq 0 (
        echo Error compiling shader: !SHADER_FILE!
        exit /b 1
    )
)

echo Shader compilation complete.