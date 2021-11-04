@echo off

set ROOT_PATH=%cd%
set SERVER_DIR=%ROOT_PATH%\Server
set CLIENT_DIR=%ROOT_PATH%\Client
set FLAGS=-Wall -Werror -Wextra

if %1 == clean goto :clean
if %1 == all goto :all

:clean
	del %SERVER_DIR%\server.exe
	del %CLIENT_DIR%\client.exe
goto :eof

:all
	gcc %FLAGS% -o %SERVER_DIR%\server.exe functions.c %SERVER_DIR%\server.c -lws2_32
	gcc %FLAGS% -o %CLIENT_DIR%\client.exe functions.c %CLIENT_DIR%\client.c -lws2_32
goto :eof