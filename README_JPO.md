# JPO Micropython Readme

Micropython has been adapted to work with the JPO board, based on Pi Pico (RP2).


# Building Micropython RP2 port in Windows
Replace `/d/danilom_micropython` below with the location of the Micropython github repository on the your machine.

## Configuration

1. Download and install MSYS2 from https://www.msys2.org/

I used the latest, `msys2-x86_64-20230718.exe`
Install MSYS2 to the default dir, `C:\msys64`

2. Update MSYS
Start the `msys2.exe` shell and update msys:
```
pacman -Syuu
```
Answer yes to any prompts. After a partial update, it might close the shell. In that case, open it again and do one more: 
```
pacman -Syuu
```

3. Install packages
Not sure if all these are needed, didn't have time to prune down the list

One:
```
pacman -S --needed make pkg-config python3 mingw-w64-i686-cmake mingw-w64-i686-gcc
```
Two:
```
pacman -S base-devel mingw-w64-x86_64-arm-none-eabi-toolchain mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```
Three
```
pacman -S git
```

4. Build the Micropython cross-compiler

Open the `mingw64.exe` shell (*not msys2.exe!*), do:
```
cd /d/danilom_micropython/ports/rp2/mpy-cross
make
```
Cross-compiler build should to complete successfully.

5. Make a symbolic link to PICO SDK
Replace `/c/VSARM/sdk/pico/pico-sdk/` with the location of your SDK. 
```
cd /d/danilom_micropython/lib
ln -s /c/VSARM/sdk/pico/pico-sdk/ .
```

6. Make git submodules
```
cd /d/danilom_micropython/ports/rp2
make BOARD=PICO submodules
```

## Building

1. Open the `mingw64.exe` shell, do `cd ports/rp2` and run `./jpo-build.sh`. 

Specify no args for both builds, or `dbgr`/`fast` for a specific build, `clean` to remove `build` folders.

This should generate the `firmware.elf` binary in `build-jpo-dbgr` and/or `build-jpo-fast`,
and also copy it to 

2. To flash the binary, connect the brain and in **Windows** command prompt (or powershell), do `jpo sw mpy -f` (or -fast)

Alternatively, do:
```
cd D:\danilom_micropython\ports\rp2\build-jpo-dbgr\
jpo flash
```

## Tools configuration

### Using tools on Windows path from MSYS

To use tools on the Windows path, edit `C:\msys64\mingw64.ini` (or another file like `msys2.ini`) and uncomment the line:
```
MSYS2_PATH_TYPE=inherit
```
Save the file, restart the shell if open.

When starting the shell from the script, instead use:
```
.\msys2_shell.cmd -mingw64 -full-path
```

!! To invoke JPO tools, type `jpo.cmd` (just `jpo` doesn't work)

### Setting up in VSCode

See https://www.msys2.org/docs/ides-editors/

User settings are in %USERPROFILE%\AppData\Roaming\Code\User\settings.json

To `settings.json` (project or user), add:
```json
    "terminal.integrated.profiles.windows": {
        "MSYS2 MINGW64 (Micropython)": {
            "path": "cmd.exe",
            "args": [
                "/c",
                "C:\\msys64\\msys2_shell.cmd -defterm -here -no-start -mingw64 -full-path"
            ]
        }
    }
```

### Setting up in Windows Terminal

See https://www.msys2.org/docs/terminals/

Open Windows terminal, click the dropdown (+v), select Settings, click "Open JSON file", add the following:
(replace or remove "startingDirectory" as needed)
```json
    "profiles":
    ...
        "list":
        ...
            {
                "guid": "{17da3cac-b318-431e-8a3e-7fcdefe6d114}",
                "hidden": false,
                "name": "MSYS2 MINGW64 (Micropython)",
                "commandline": "C:\\msys64\\msys2_shell.cmd -defterm -here -no-start -mingw64 -full-path",
                "startingDirectory": "D:/danilom_micropython/ports/rp2",
                "icon": "C:/msys64/mingw64.ico"
            },
```