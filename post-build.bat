@echo off

REM Run from root directory!
if not exist "%cd%\cmake-build-debug\assets\shaders\" mkdir "%cd%\cmake-build-debug\assets\shaders"
if not exist "%cd%\cmake-build-release\assets\shaders\" mkdir "%cd%\cmake-build-release\assets\shaders"

echo "Compiling shaders..."

echo "assets/shaders/shader.vert -> assets/shaders/shader.vert.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/shader.vert -o assets/shaders/shader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "assets/shaders/shader.frag -> assets/shaders/shader.frag.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/shader.frag -o assets/shaders/shader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Copying assets..."
echo xcopy "assets" "cmake-build-debug\assets" /h /i /c /k /e /r /y
xcopy "assets" "cmake-build-debug\assets" /h /i /c /k /e /r /y

echo xcopy "assets" "cmake-build-release\assets" /h /i /c /k /e /r /y
xcopy "assets" "cmake-build-release\assets" /h /i /c /k /e /r /y
echo "Done."