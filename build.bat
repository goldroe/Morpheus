@echo off

IF NOT EXIST .build MKDIR .build
PUSHD .build
CL -nologo -FC -Zi -WX -W4 -wd4100 -wd4189 -wd4530 -wd4101 -DDEVELOPER ..\src\win32_morpheus.cpp -Fe:Morpheus.exe -link -INCREMENTAL:NO user32.lib gdi32.lib d3d11.lib d3dcompiler.lib winmm.lib
MOVE *.exe ..
POPD
