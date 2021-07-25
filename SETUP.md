
Followed instructions at:

* https://github.com/raspberrypi/pico-sdk

Installed cmake and llvm: https://github.com/a-h/dotfiles/commit/458ac8189f7c2f5f5c215869632f1a09f9e5f675#diff-090c1bf82766c7cb9d997ad6141afed010465f6e7630f69c9b099765d386320a

Clone the SDK into https://github.com/raspberrypi/pico-sdk
Added it to the environment under PICO_SDK_PATH (see dotfiles).

Copy the https://github.com/raspberrypi/pico-sdk/blob/master/external/pico_sdk_import.cmake into this directory.

wget https://raw.githubusercontent.com/raspberrypi/pico-sdk/master/external/pico_sdk_import.cmake

Set up CMakeLists.txt to download from Github.

```
cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(my_project)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
```

Made build directory, and ran `cmake ..`

```
mkdir build
cd build
cmake ..
```

```
Using PICO_SDK_PATH from environment ('/Users/adrian/github.com/raspberrypi/pico-sdk')
PICO_SDK_PATH is /Users/adrian/github.com/raspberrypi/pico-sdk
Defaulting PICO_PLATFORM to rp2040 since not specified.
Defaulting PICO platform compiler to pico_arm_gcc since not specified.
-- Defaulting build type to 'Release' since not specified.
PICO compiler is pico_arm_gcc
CMake Error at /Users/adrian/github.com/raspberrypi/pico-sdk/cmake/preload/toolchains/find_compiler.cmake:28 (message):
  Compiler 'arm-none-eabi-gcc' not found, you can specify search path with
  "PICO_TOOLCHAIN_PATH".
Call Stack (most recent call first):
  /Users/adrian/github.com/raspberrypi/pico-sdk/cmake/preload/toolchains/pico_arm_gcc.cmake:20 (pico_find_compiler)
  /nix/store/nlq0wyxkqmx5n6bsf516i0y8k7zgjw4c-cmake-3.19.3/share/cmake-3.19/Modules/CMakeDetermineSystem.cmake:123 (include)
  CMakeLists.txt:11 (project)


CMake Error: CMake was unable to find a build program corresponding to "Unix Makefiles".  CMAKE_MAKE_PROGRAM is not set.  You probably need to select a different build tool.
CMake Error: CMAKE_C_COMPILER not set, after EnableLanguage
CMake Error: CMAKE_CXX_COMPILER not set, after EnableLanguage
-- Configuring incomplete, errors occurred!
```

Downloaded package here: 

https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

Set the environment variable:

```
export PICO_TOOLCHAIN_PATH=/Applications/ARM
```

Ran.
```
make ..
```

Then 

```
make
```

Plug in Raspberry Pi and copy the compiled output over.

```
cp hello_world.uf2 /Volumes/RPI-RP2
```

Serial didn't work! Added LED to check.

Googling, realised needed `pico_enable_stdio_usb(hello_world 1)` in CMakeLists.

Then got this error:

```
PICO_SDK_PATH is /Users/adrian/github.com/raspberrypi/pico-sdk
PICO platform is rp2040.
PICO target board is pico.
Using board configuration from /Users/adrian/github.com/raspberrypi/pico-sdk/src/boards/include/boards/pico.h
CMake Warning at /Users/adrian/github.com/raspberrypi/pico-sdk/src/rp2_common/tinyusb/CMakeLists.txt:10 (message):
  TinyUSB submodule has not been initialized; USB support will be unavailable

  hint: try 'git submodule update --init' from your SDK directory
  (/Users/adrian/github.com/raspberrypi/pico-sdk).


-- Configuring done
-- Generating done
-- Build files have been written to: /Users/adrian/github.com/a-h/pico-mouse/build
```

Did what it said:

```
cd /Users/adrian/github.com/raspberrypi/pico-sdk
git submodule update --init
```

## Getting autocomplete to work with vim

https://ianding.io/2019/07/29/configure-coc-nvim-for-c-c++-development/

Tried to setup CCLS to get autocomplete.

https://github.com/MaskRay/ccls/wiki/Project-Setup

Need to add this somewhere: `-DCMAKE_EXPORT_COMPILE_COMMANDS=YES`

Worked out I could add it to CMakeLists.txt

```
# Export build commands for the CCLS language server.
# See https://github.com/MaskRay/ccls/wiki/Project-Setup
# See https://stackoverflow.com/questions/20059670/how-to-use-cmake-export-compile-commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

```
ln -s build/compile_commands.json .
```

Used this to get my OLED screen running, along with this:

https://github.com/matiasilva/pico-examples/tree/oled-i2c/i2c/oled_i2c
https://iotexpert.com/debugging-ssd1306-display-problems/

Built a font with Google sheets.
