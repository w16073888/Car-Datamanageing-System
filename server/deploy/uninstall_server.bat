@echo off
rem ============================================================
rem  4s-server 开机自启 取消脚本
rem ============================================================
setlocal
set "STARTUP=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
set "VBS=%STARTUP%\4s-server-autostart.vbs"
if exist "%VBS%" (
    del "%VBS%"
    echo [完成] 已取消 4s-server 开机自启。
) else (
    echo [提示] 未找到自启项，无需取消。
)
pause
exit /b 0
