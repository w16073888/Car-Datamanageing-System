@echo off
rem ============================================================
rem  4s-server 开机自启注册脚本（启动文件夹 + VBS 隐藏启动器）
rem  用法：  install_server.bat              注册自启（幂等，可重复运行）
rem          install_server.bat uninstall    取消自启
rem ============================================================
setlocal

set "DEPLOY=%~dp0"
set "EXE=%DEPLOY%4s-server.exe"

if /i "%~1"=="uninstall" goto :uninstall

if not exist "%EXE%" (
    echo [错误] 未找到 4s-server.exe：
    echo   %EXE%
    echo 请把本脚本放在 server\deploy 目录里运行。
    pause
    exit /b 1
)

set "STARTUP=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
if not exist "%STARTUP%" mkdir "%STARTUP%"
set "VBS=%STARTUP%\4s-server-autostart.vbs"

>  "%VBS%" echo Set ws = CreateObject("WScript.Shell")
>> "%VBS%" echo ws.Run "%EXE%", 0, False

echo.
echo [完成] 已注册开机自启。
echo   启动器: %VBS%
echo   启动程序: %EXE%
echo   下次登录时自动隐藏运行（无黑窗口）。
echo.
echo [提醒] 请确认 MySQL 服务已设为开机自启，否则服务端会一直空转重试。
echo [取消] 运行 uninstall_server.bat，或直接删除上面那个 vbs 文件。
echo.
pause
exit /b 0

:uninstall
set "STARTUP=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
set "VBS=%STARTUP%\4s-server-autostart.vbs"
if exist "%VBS%" (
    del "%VBS%"
    echo [完成] 已取消开机自启。
) else (
    echo [提示] 未找到自启项，无需取消。
)
pause
exit /b 0
