@echo off
echo Script to generate JPO Micropython stubs used by VS Code for autocompletion/errors/tooltip docs
echo.
echo Requires a "stubber" tool from https://github.com/Josverl/micropython-stubber, to install (in admin console):
echo     pip install -U micropython-stubber
echo.

set OUT_PATH=%JPO_PATH%\resources\py_stubs

if EXIST "%OUT_PATH%" GOTO OUT_PATH_EXISTS 
  echo Error: environment variable JPO_PATH not set or "%OUT_PATH%" does not exist
  exit -1
:OUT_PATH_EXISTS

cd %~dp0/repos

if EXIST micropython GOTO MPY_SYMLINK_EXISTS
  echo Creating symlink "repos/micropython"
  REM 4 levels up is the root of the micropython repo
  mklink /D micropython ..\..\..\..\
  if %errorlevel%==0 GOTO MPY_SYMLINK_EXISTS
    echo ERROR: Right click command prompt and open using "Run as Administrator"
    exit /b %errorlevel%
:MPY_SYMLINK_EXISTS

if EXIST micropython-lib GOTO LIB_SYMLINK_EXISTS
  echo Creating symlink "repos/micropython-lib"

  if EXIST \micropython-lib GOTO LIB_DIR_EXISTS
    echo ERROR: \micropython-lib directory does not exist, check out the repo to the root of this drive
    exit /b -1
  :LIB_DIR_EXISTS

  mklink /D micropython-lib \micropython-lib
  if %errorlevel%==0 GOTO LIB_SYMLINK_EXISTS
    echo ERROR: Right click command prompt and open using "Run as Administrator"
    exit /b %errorlevel%
:LIB_SYMLINK_EXISTS

cd ..
REM GOTO SKIP_STUBBER
  echo Removing old stubs...
  rd /s /q stubs
  md stubs

  echo === get-docstubs
  stubber get-docstubs
  if %errorlevel% neq 0 exit %errorlevel%

  echo === get-frozen
  stubber get-frozen
  if %errorlevel% neq 0 exit %errorlevel%
:SKIP_STUBBER

echo Writing to "%OUT_PATH%"...
rd /s /q %OUT_PATH%

md %OUT_PATH%\docstubs
xcopy /e stubs\micropython-preview-docstubs\*.pyi "%OUT_PATH%\docstubs\"

md %OUT_PATH%\frozen\rp2
xcopy /e stubs\micropython-preview-frozen\rp2\GENERIC\*.pyi "%OUT_PATH%\frozen\rp2\"

echo Done, see "%OUT_PATH%".