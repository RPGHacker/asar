# Asar
[![Windows builds (AppVeyor)](https://ci.appveyor.com/api/projects/status/github/RPGHacker/asar?svg=true)](https://ci.appveyor.com/project/RPGHacker/asar) ![Ubuntu build (GitHub Actions)](https://github.com/RPGHacker/asar/actions/workflows/cmake.yml/badge.svg)

Asar is an SNES assembler designed for applying patches to existing ROM images, or creating new ROM images from scratch. It supports 65c816, SPC700, and Super FX architectures. It was originally created by Alcaro, modelled after [xkas v0.06](https://www.romhacking.net/utilities/269/) by byuu.

For a guide on using Asar (including how to write patches), see [`README.txt`](https://github.com/RPGHacker/asar/blob/master/README.txt). This readme was made with tool programmers and contributors in mind.

## Building
You can build Asar with [CMake](https://cmake.org). On Linux, the most basic build would look like `cmake src && make`. On Windows, using Visual Studio, you would do `cmake src`, then open the project file it generates in Visual Studio and click Build. Alternately, you might be able to use Visual Studio's [CMake integration](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio).

If you'd rather not build from source, check out the [Releases](https://github.com/RPGHacker/asar/releases) page.

## Asar DLL
Asar can also be built as a DLL. This makes it easier and faster to use in other programs (such as a sprite insertion tool). You can find documentation on the DLL API in the respective bindings (asardll.h, asar.cs, asar.py).

## Asar as a static library
Asar can also be built as a static library. All "out-facing" functions are in interface-lib.h. This is useful for embedding Asar in other programs which don't want to use DLLs. The easiest way to add asar as a static library to your project is to add it as a git submodule 

`git submodule add https://github.com/RPGHacker/asar <path-to-subdir>`

then add the following to your CMakeLists.txt:
```CMake
add_subdirectory(<path-to-subdir>/src)
get_target_property(ASAR_INCLUDE_DIR asar-static INCLUDE_DIRECTORIES)
include_directories(${ASAR_INCLUDE_DIR})
target_link_libraries(YourTarget PUBLIC asar-static)
```
to be able to include the header files. It is also recommended to turn off every build in target in asar except the static one using the appropriate CMake options. You will need to make sure that your project has an Asar compatible license.

## Folder layout
* `docs` contains the source of the manual and changelog.
  (You can view an online version of the manual [here](https://rpghacker.github.io/asar/manual/) and an online version of the changelog [here](https://rpghacker.github.io/asar/changelog/)).
* `ext` contains syntax highlighting files for Notepad++ and Sublime Text
* `src`
  * `asar` contains the source code of the main app and DLL
  * `asar-tests` contains code for the testing application (both the app test and DLL test)
  * `asar-dll-bindings` contains bindings of the Asar DLL to other languages (currently C/C++, C# and Python)
* `tests` contains tests to verify Asar works correctly

## Test format
*Please note that these tests are intended for use with Asar's test suite. Only contributors will need to use this functionality - people who just want to create and apply patches don't need to worry about it.*

At the beginning of your ASM files, you can write tests to ensure the correct values were written to the ROM after patching is complete. (It's common to use a SMW ROM, but there's also a dummy ROM included that should work with all tests.)

These two characters should precede each test line, so that Asar sees them as comments and ignores them.
```
;`
```

* 5-6 hex digits - the ROM offset to check 
  * Specify it as a PC address, not a SNES address
  * When left blank, it defaults to `0x000000`
* 2 hex digits - a byte for it to check for 
  * You can specify more than one, like in the examples below, and it will automatically increment the offset.
* A line starting with `+` tells the testing app to patch the SMW ROM instead of creating a new ROM
* `#` followed by a decimal number tells the testing app to apply the patch that number of times repeatedly
* `errE{name}` and `warnW{name}` (where `{name}` is the name of an error or warning) means that the test is expected to throw that specific error or warning while patching. The test will succeed only if the number and order of errors and warnings thrown exactly matches what's specified here. Be wary that Asar uses multiple passes and throws errors and warnings across multiple of them. This can make the actual order in which errors and warnings are thrown a bit unintuitive.

In addition to the format mentioned above, it's also possible to check for user prints a patch is expected to output (by `print`, `error`, `warn` or `assert` commands). This is done by starting the line with one of the following sequences:
```
;E>
;W>
;P>
```
Where E is for errors/asserts, W is for warnings and P is for prints. Following this sequence, every character up to the end of the current line is a part of the expected string to be output. Note that the test suite also verifies the order of prints within the respective type. So if your patch is expected to output two user-defined errors, they need to be specified exactly in the order in which they are expected to be output.

**Example tests:**

This line tests that the bytes `5A`, `40` and `00` (in that order) were written to the start of the ROM.
```
;`5A 40 00
```

This line tests that `22`, `20`, `80` and `90` were written to the ROM offset `0x007606`.
```
;`007606 22 20 80 90
```

This line tests that assembling the patch throws error `Eunknown_command` twice and warning `Wfeature_deprecated` once.
```
;`errEunknown_command
;`errEunknown_command
;`warnWfeature_deprecated
```

This line tests that the byte `FF` was written to the start of the ROM, that the string `This is a print.` was printed and that the string `This is a user error.` was output via the error command (which itself also causes error `Eerror_command` to be thrown once).
```
;`FF
;P>This is a print.
;E>This is a user error.
;`errEerror_command
```
