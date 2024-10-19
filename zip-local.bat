@echo off

IF "%1" == "clean" (
    rmdir /S /Q build\windows
)

pushd %CD%
cd build/windows
call ./build19.bat
zip -9 gkNextRenderer-windows-git.zip ./bin/*.* ./assets/locale/*.* ./assets/shaders/*.* ./assets/textures/*.* ./assets/fonts/*.* ./assets/models/*.* ./*.bat
move /Y gkNextRenderer-windows-git.zip ..\..\
popd
move /Y gkNextRenderer-windows-git.zip i:\