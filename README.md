# Asar

Asar is a SNES assembler designed for applying patches to existing roms. It was originally created by Alcaro.

For a guide on using Asar (including writing patches), see README.txt. This README is made for programmers.

## Building

Asar is built using CMake. On Linux, the most basic build would look like `cmake src && make`. On Windows, using Visual Studio you would do `cmake src`, then open the generated project in Visual Studio and click Build.

## Asar DLL

Asar can also be built as a DLL. This makes it easier to use Asar from other programs (such as a sprite insertion tool). You can find documentation on the functions in the DLL in the bindings (asardll.h, asar.cs, asar.py).

## Folder layout

* docs contains the source of the manual and changelog.
* ext currently only contains syntax highlighting files for Notepad++ and Sublime Text.
* src contains the source code of Asar.
  * In there, asar contains the source code of the main app and DLL
  * asar-app-test contains code for running the built-in test suite using asar.exe (the standalone application)
  * asar-dll-test contains code for running the built-in test suite using asar.dll (the library)
  * asar-tests-shared contains common code for the above
  * asar-dll-bindings contains bindings of the Asar DLL to other languages (currently C/C++, C# and Python)
* tests contains tests to verify Asar works correctly.

## Test format

Tests are mostly just asm files to be patched to a known ROM (currently SMW) with a bit of special syntax to verify that the correct values are written to the ROM. Tests contain a number of lines starting with `;@` in the beginning of the file. All of these lines are removed before actually applying the patch. On those lines:

* 2 hexadecimal digits means that the byte specified will be expected in the rom at the current location.
* 5-6 hex digits specify the location (in the file, not snes position!) to expect the following byte at.
* a line starting with `+` tells the testing app to patch this file to the SMW rom instead of a blank file.
* `err` means that this test is expected to throw an error when patching. The test will fail when the file is successfully patched.

If no starting position is specified, the bytes will be expected at the start of the ROM.

For example, special lines
```
;@5A 40 00
```
expect the bytes 5A, 40 and 00 to be at the start of the output ROM after patching (in that order).

```
;@07606 22 20 80 90
```
expects the bytes 22, 20, 80 and 90 at ROM offset $007606.
