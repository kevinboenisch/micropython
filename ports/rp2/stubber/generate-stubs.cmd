@echo off
echo Script to generate JPO Micropython stubs used by VS Code for autocompletion/errors/tooltip docs
echo.
echo Requires the "stubber" tool from https://github.com/Josverl/micropython-stubber, to install (in admin console):
echo     pip install -U micropython-stubber
echo.
echo In case of a "bad credentials" error, install stubber again using the above line.

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

  md repos\micropython-stubs
  md repos\micropython-stubs\stubs

  echo === get-docstubs
  stubber get-docstubs
  if %errorlevel% neq 0 exit %errorlevel%

  echo === get-frozen
  stubber get-frozen
  if %errorlevel% neq 0 exit %errorlevel%
:SKIP_STUBBER

echo Done.