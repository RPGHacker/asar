# Standard Includes

Aside from passing include search paths to Asar via the command line, it's possible to also do so via a file called `stdincludes.txt`. When a file with this name exists next to the Asar executable, Asar automatically opens it and adds every line in it as an include search path (trailing and leading whitespace on a line is ignored, as are lines containing only whitespace). Absolute and relative paths are supported. Relative paths are considered relative to the TXT file. The purpose of this file is to make it easier to distribute standard code libraries for use with Asar by making it possible to just unpack the contents of a ZIP file or similar directly into the Asar directory. Note that include search paths passed in via the command line get priority over paths parsed from this TXT file. See section [Includes](./includes.md) for details on include search paths.

All of the examples below are valid:

```
C:/asm/stdlib

    ./debug
../../my_game/libraries
        test/
```
