# Asar

Asar is an SNES assembler designed for applying patches to existing ROMs or creating new ROMs from scratch. It supports 65c816, SPC700 and Super FX architextures and was originally created by Alcaro, based on xkas v0.06 by byuu.

For a guide on using Asar (including writing patches), see README.txt. This README is made for tool programmers and contributors.

## Building

Asar is built using CMake. On Linux, the most basic build would look like `cmake src && make`. On Windows, using Visual Studio you would do `cmake src`, then open the generated project in Visual Studio and click Build.

## Asar DLL

Asar can also be built as a DLL. This makes it easier and faster to use Asar with other programs (such as a sprite insertion tool). You can find documentation on the DLL API in the respective bindings (asardll.h, asar.cs, asar.py).

## Folder layout

* docs contains the source of the manual and changelog.
  (You can view an online version of the manual [here](https://rpghacker.github.io/asar/manual/) and an online version of the changelog [here](https://rpghacker.github.io/asar/changelog/)).
* ext currently only contains syntax highlighting files for Notepad++ and Sublime Text.
* src contains the source code of Asar.
  * In there, asar contains the source code of the main app and DLL
  * asar-app-test contains code for running the built-in test suite using asar.exe (the standalone application)
  * asar-dll-test contains code for running the built-in test suite using asar.dll (the library)
  * asar-tests-shared contains code shared between the two above
  * asar-dll-bindings contains bindings of the Asar DLL to other languages (currently C/C++, C# and Python)
* tests contains tests to verify Asar works correctly.

## Test format

Tests are mostly just asm files to be patched to a known ROM (currently SMW) with a bit of special syntax to verify that the correct values are written to the ROM. Tests contain a number of lines starting with `;@` in the beginning of the file. All of these lines are removed before actually applying the patch. On those lines:

* 5-6 hex digits specify the offset in the ROM (as a PC address, not a an SNES address!) to expect the following bytes at.
* 2 hexadecimal digits means that the byte specified will be expected in the ROM at the current offset (automatically increases the current offset).
* a line starting with `+` tells the testing app to patch this file to the SMW ROM instead of creating a new ROM.
* `err` means that this test is expected to throw an error when patching. The test will fail if no error is thrown.

When no offset is specified, a default offset of 000000 is used.

For example, special lines
```
;@5A 40 00
```
expect the bytes 5A, 40 and 00 to be at the start of the output ROM (offset 000000) after patching (in that order).

```
;@07606 22 20 80 90
```
expects the bytes 22, 20, 80 and 90 at ROM offset $007606.
