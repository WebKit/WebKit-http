@echo off

set PrivateHeadersDirectory=%CONFIGURATIONBUILDDIR%\include\private
set PGOPrivateHeadersDirectory=%CONFIGURATIONBUILDDIR%\..\Release_PGO\include\private

if "%1" EQU "clean" goto :clean
if "%1" EQU "rebuild" call :clean

echo Copying WTF headers...
for %%d in (
    wtf
    wtf\dtoa
    wtf\text
    wtf\threads
    wtf\unicode
    wtf\unicode\icu
) do (
    mkdir "%PrivateHeadersDirectory%\%%d" 2>NUL
    xcopy /y /d ..\%%d\*.h "%PrivateHeadersDirectory%\%%d" >NUL
)

# FIXME: Why is WTF copying over create_hash_table?
echo Copying other files...
for %%f in (
    ..\JavaScriptCore\create_hash_table
    wtf\text\AtomicString.cpp
    wtf\text\StringBuilder.cpp
    wtf\text\StringImpl.cpp
    wtf\text\WTFString.cpp
) do (
    echo F | xcopy /y /d ..\%%f "%PrivateHeadersDirectory%\%%f" >NUL
    echo F | xcopy /y /d ..\%%f "%PGOPrivateHeadersDirectory%\%%f" >NUL
)

goto :EOF

:clean

echo Deleting copied files...
if exist "%PrivateHeadersDirectory%" rmdir /s /q "%PrivateHeadersDirectory%" >NUL
if exist "%PGOPrivateHeadersDirectory%" rmdir /s /q "%PGOPrivateHeadersDirectory%" >NUL
