@echo off
rem ============================================================
rem  garagedb 数据备份脚本（双击运行即可）
rem  备份文件保存到：本脚本同目录下的 backup\ 文件夹
rem  自动保留最近 KEEP_DAYS 天，更旧的自动删除
rem ============================================================
setlocal enabledelayedexpansion

rem ============ 配置区（改这里的值） ============
set "DB_USER=garage"
set "DB_PWD=123456"
set "DB_NAME=garagedb"
set "KEEP_DAYS=14"
rem  如果找不到 mysqldump.exe（PATH 和常见安装路径都不对），
rem  取消下面这行注释并改成你机器上 MySQL 的 bin 目录：
rem set "MYSQL_BIN=C:\mysql-5.7.44-winx64\bin"
rem ==============================================

set "BACKUP_DIR=%~dp0backup"
if not exist "%BACKUP_DIR%" mkdir "%BACKUP_DIR%"

rem ---------- 定位 mysqldump ----------
set "DUMP="
if defined MYSQL_BIN if exist "%MYSQL_BIN%\mysqldump.exe" set "DUMP=%MYSQL_BIN%\mysqldump.exe"
if defined DUMP goto :have_dump

where mysqldump >nul 2>&1
if not errorlevel 1 (
    set "DUMP=mysqldump"
    goto :have_dump
)

for %%p in (
    "C:\mysql-5.7.44-winx64\bin"
    "D:\mysql-5.7.44-winx64\bin"
    "C:\Program Files\MySQL\MySQL Server 5.7\bin"
    "C:\Program Files\MySQL\MySQL Server 5.6\bin"
    "C:\mysql\bin"
) do (
    if exist "%%~p\mysqldump.exe" (
        set "DUMP=%%~p\mysqldump.exe"
        goto :have_dump
    )
)

echo.
echo [错误] 找不到 mysqldump.exe。
echo 请把 MySQL 的 bin 目录填到脚本顶部 MYSQL_BIN 那行（取消注释并修改）。
pause
exit /b 1

:have_dump
echo 备份工具: %DUMP%

rem ---------- 生成时间戳 YYYYMMDD_HHMMSS ----------
set "STAMP="
for /f "skip=1 tokens=1" %%a in ('wmic os get localdatetime') do if not defined STAMP set "STAMP=%%a"
if "%STAMP%"=="" (
    for /f "tokens=1-3 delims=/ " %%a in ('echo %date%') do set "D1=%%a"& set "D2=%%b"& set "D3=%%c"
    for /f "tokens=1-2 delims=:. " %%a in ('echo %time%') do set "T1=%%a"& set "T2=%%b"
    set "STAMP=!D1!!D2!!D3!_!T1!!T2!"
)
set "OUT=%BACKUP_DIR%\%DB_NAME%_%STAMP:~0,8%_%STAMP:~8,6%.sql"

rem ---------- 执行备份 ----------
set "MYSQL_PWD=%DB_PWD%"
echo 正在备份数据库 %DB_NAME% ...
"%DUMP%" --single-transaction --no-tablespaces --routines --default-character-set=utf8mb4 -u "%DB_USER%" "%DB_NAME%" --result-file="%OUT%"
if errorlevel 1 goto :dumpfail
set "MYSQL_PWD="

rem ---------- 清理超过 KEEP_DAYS 天的旧备份 ----------
forfiles /p "%BACKUP_DIR%" /m %DB_NAME%_*.sql /d -%KEEP_DAYS% /c "cmd /c del @path" 2>nul

echo.
echo [完成] 备份成功！
for %%f in ("%OUT%") do echo   备份文件: %%f
for %%f in ("%OUT%") do echo   大小: %%~zf 字节
echo 备份目录: %BACKUP_DIR%
echo 最近 %KEEP_DAYS% 天内的旧备份已自动清理。
pause
exit /b 0

:dumpfail
echo.
echo [错误] 备份失败。
echo 请检查 MySQL 服务是否在运行，以及顶部 DB_USER 和 DB_PWD 是否正确。
echo 上面的 mysqldump 报错信息请仔细查看。
pause
exit /b 1
