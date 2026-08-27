@echo off
set PATH=D:\Qt\Tools\mingw1310_64\bin;D:\QT5.15.2\qt515\bin;%PATH%
cd /d D:\temmpcode\4s\_scratch\pdf_render
D:\QT5.15.2\qt515\bin\qmake.exe pdf_render.pro
if errorlevel 1 exit /b 1
mingw32-make -j8
set MAKE_EXIT=%ERRORLEVEL%
echo [RENDER] MAKE_EXIT=%MAKE_EXIT%
if not "%MAKE_EXIT%"=="0" exit /b %MAKE_EXIT%
release\pdf_render.exe
exit /b 0
