# Asar Changelog

## v2.0.0

**Release:** ???

### Contributors:

-   randomdude999
-   RPG Hacker
-   p4plus2

### New features:

-   UTF-8 support: Asar now expects all source files to be UTF-8-encoded. Unicode code points (encoded as UTF-8) are now supported in table files, math commands, tables etc. Unicode file names are now also supported on Windows. (randomdude999, RPG Hacker)
-   Added support for nested macro definitions. (RPG Hacker)
-   Improved readability of some error messages. (RPG Hacker)
-   Generally improved error formatting of Asar and added the `--full-error-stack` option to display complete call stacks. (RPG Hacker)
-   Added multiline comments using `;[[ comment ]]` (randomdude999)
-   Freespaces have a bunch of new features which make them more useful outside of SMW hacking - new options for disabling RATS tags, looking for code in a specific bank or a specific part of ROM. Along with this, freespace allocation should now be a bit more efficient, and the 125 freespace limit has been lifted. (randomdude999)
-   Allowed spaces in math. (p4plus2)
-   A struct can now be used directly like a regular label. (p4plus2)
-   Greatly improved "invalid instruction" errors, which now tell you when an addressing mode doesn't exist for the current instruction, instead of saying "unknown command" for everything. (randomdude999)
-   `autoclean` can now be used with more instructions, allowing such contraptions as `autoclean cmp label,x` if desired. (randomdude999)

### Bug fixes:

-   Asar no longer strips just any white space from defines, allowing them to more closely reflect user intent. (p4plus2)
-   Using `org $xxxxxx : db $00` to expand the ROM to a specific size will now allow the freespace finder to use the newly created space. (randomdude999)
-   In `norom`, using `base` will now no longer give bank cross errors for the "real" position, since there are no real banks in norom. (randomdude999)
-   Better branch instruction handling: some legal but previously rejected branches (especially wrapping around bank borders) are now accepted, and some illegal branches are now properly rejected. (binary1230)
-   A bunch more bugfixes and crash fixes that are too minor to list here

### Removed features:

-   All features that were deprecated in Asar v1.90 or earlier.

- - -

## v1.90

**Release:** ???

### Contributors:

-   RPG Hacker
-   randomdude999
-   p4plus2
-   Atari2
-   Alcaro
-   spooonsss

### Notes:

-   The primary purpose of this release is to be a stepping stone towards Asar 2.0. For this purpose, a lot of features have been deprecated and will now throw warnings. Please fix any of those warnings you come across in your patches to assure they will still assemble in Asar 2.0.

### New features:

-   The `error`, `warn` and `assert` commands now support the same functions as the `print` command. (RPG Hacker)
-   Static labels (i.e. labels that don't move between passes) can now be used in more places, including if statements. (RPG Hacker)
-   Asar can be built as a static library. (Atari2)
-   Asar now uses named warnings and errors instead of magic numbers as identifiers. (p4plus2, RPG Hacker)
-   Variadic macro parameters now use the syntax `<...[math]>`, which makes them less ambiguous and helps prevent syntax parsing bugs. (RPG Hacker)
-   HiROM, ExHiROM, and ExLoROM mappers can now use `freecode`. (p4plus2)
-   `check bankcross` now allows disabling checking for half-bank crossings (going from $7FFF to $8000). (p4plus2)
-   New `realbase()` function: returns the current SNES address that is being written to. (p4plus2)
-   Namespaces can now be saved using `pushns` and `pullns`. (p4plus2)
-   It's now possible to use macro arguments and macro labels in files that are `incsrc`'d from macros. (randomdude999)
-   Added `for` loops, used like `for i = 1..10`. These are more convenient than while loops in most cases and are the main replacement for `rep`. (randomdude999)
-   `incbin` now has a new syntax for including a range of the target file which looks like the for loop range syntax and is less ambiguous. (randomdude999)
-   Added `spcblock` feature as a replacement for `spc-inline`, which allows defining blocks of SPC data, but in a more flexible way that can easily be extended in the future. (p4plus2)
-   Added the `--error-limit` option, which allows raising the limit on the number of errors that Asar will print before stopping. (randomdude999)

### Bug fixes:

-   Variadic arguments in macros can now also take zero arguments, which can be used to implement optional arguments. (RPG Hacker)
-   Escaping quotes in macro calls now works more reliably. (RPG Hacker)
-   Macro calls & definitions no longer break as easily from including whitespace. (RPG Hacker)
-   For invalid table files, Asar now prints the line number of the first invalid entry. (RPG Hacker)
-   Addr-to-line mappings now include `autoclean jml/jsl` commands, pseudo opcodes like `asl #4`, and most other data-writing commands like `db/dw/dl`. (spooonsss, RPG Hacker, randomdude999)
-   `'''` and `';'` are now valid can now be used without causing errors. (randomdude999, RPG Hacker)
-   Fixed some edge case bugs in Asar's virtual filesystem (usable via the DLL) on Windows. (Atari2)
-   The `--version` commandline flag now causes asar to exit afterwards, which is the expected behavior for command-line flags. (Alcaro)
-   Fixed some bugs with the label optimizer, making it optimize better. (p4plus2)
-   Fixed freespaces sometimes being a few bytes too long. (randomdude999)
-   Assigning labels with = now puts them in the right namespace. (p4plus2)
-   Fixed some memory leaks and possible crashes in the DLL. (randomdude999)
-   Fixed some phantom errors when using forward labels. (randomdude999)
-   Asar should now allow as much recursion as system resources allow, and then throw a nice error instead of crashing. (Atari2.0, randomdude999, p4plus2)
-   `optimize address` now works a bit more consistently (new behavior also properly documented in manual). (randomdude999)
-   `freespacebyte` command to set the byte value used for searching for freespace and expanding the ROM. (randomdude999)

### Deprecated features:

-   Warning and error IDs: Use new name strings instead.
-   xkas compatibility mode: port your patch to asar instead. Everything xkas compatibility mode does should already throw warnings.
-   `JMP.l`: Use `JML` instead.
-   Quoted symbolic arguments to functions (e.g. `sizeof("my_struct")`): Remove the quotes (`sizeof(my_struct)`).
-   Redefining previously defined functions.
-   `math round` and `math pri`: Use parentheses and explicit rounding where xkas-style math emulation is needed instead.
-   `if !condition` to negate conditions: Use `if not(condition)` instead.
-   While blocks ending with `endif`: Use `endwhile` instead.
-   `check bankcross on`: Use `check bankcross full` or `check bankcross half` instead.
-   `rep` to repeat commands: Use while loops, unrolled loops or for loops instead.
-   Windows-specific paths (e.g. `incsrc dir\file.asm`): Use cross-platform paths instead (`incsrc dir/file.asm`).
-   `table` command: Assign characters directly instead, like `'a' = $00`.
-   `spc700-inline`: Use `spcblock` and `endspcblock` instead.
-   `spc700-raw`: use `arch spc700` and `norom`.
-   `fastrom`: Has never actually worked and can be removed.
-   `header`: Doesn't do anything and can be removed.
-   Non-UTF-8 source files: Re-save your source files as UTF-8 in a text editor of choice.
-   `;@command` and `@command` notation: Use `command` instead.
-   Wrapping defines to resolve in quotes (e.g. `db "!define"`): The quotes aren't needed (`db !define`).
-   Single-line/inline if statements: Use full if blocks with an `if`/`endif` pair instead. (Note that you can still have an if statement entirely on one line, you just need to have an `: endif` at the end of it.)
-   `freespace fixed`: Use `freespace static` instead.
-   `<math>` syntax for variadic macro parameters: Use `<...[math]>` instead.
-   incbin with a range like `incbin file.bin:0-F`: use `incbin file.bin:$0..$10` instead. (note that the new syntax is end-exclusive.)
-   `incbin file.bin -> target`: put an `org` or `freespace` command before the incbin.
-   `if a = b`: use `if a == b` instead.
-   Comments starting with `;[[` : These mark the start of block comments in Asar 2.0, so either remove the `[[` for the time being, or make the commented line end with a `]]`.
-   Using `A` as a label name ambiguously, e.g. `lda a`: in Asar 2 this will be interpreted as trying to use the "A" addressing mode, and will give an error. You can use `A+0` if you really must refer to the label as is.
-   The `startpos` command of the `spc700-inline` architecture: use the optional `execute` argument to `endspcblock` instead.
-   Specifying the byte to use for freespace like `freecode $ff`. Use the new separate `freespacebyte` command for it.

- - -

## v1.81

**Release:** January 20, 2021

### Contributors:

-   p4plus2

### Bug fixes:

-   Prevent defines from being resolved in false branches of if statements when inside variadic macros
-   Fix a phantom error when referencing a struct before defining it

- - -

## v1.80

**Release:** January 18, 2021

### Contributors:

-   RPG Hacker
-   randomdude999
-   p4plus2
-   CypherSignal
-   LDAsuku
-   Katrina
-   Atari2.0
-   Vitor Vilela

### New features:

-   The C DLL binding for Windows should now print a few more details if loading the DLL failed. (RPG Hacker)
-   The `print` command has a new `bin()` function. Also, `bin()`, `dec()` and `hex()` now take an optional width argument that allows padding the number with zeroes. (randomdude999)
-   Labels can now be used in the condition of an `assert` command. (randomdude999)
-   The WLA symbol file now includes an address-to-line mapping, which can be used by some debuggers to provide source-level debugging. (CypherSignal)
-   The `sizeof()` and `objectsize()` functions don't need quotes around the struct name anymore. For backwards compatibilty, quotes are still allowed. (p4plus2)
-   There is a new label optimizer, which can convert RAM labels to direct page or word accesses where possible. For this there are 3 new commands: `dpbase`, `optimize dp` and `optimize address`. (p4plus2)
-   The `floor()` and `ceil()` functions have been added. (randomdude999)
-   The `bank()` and `<:` function and math operator have been added (p4plus2)
-   The `datasize()` function has been added. (p4plus2)
-   Warning if the mapper mode is changed after being set (p4plus2)
-   Major improvements to internal string handling in asar (makes asar significantly faster) (p4plus2)
-   Variadic macro support (p4plus2)
-   Support for global labels (p4plus2)
-   Errors about crossing banks have been improved, now they print the exact line where the bank border was crossed instead of saying it was "somewhere before this point". (randomdude999)
-   Add `fill align` and `skip align` (randomdude999)

### Bug fixes:

-   The Asar DLL can now be loaded in Windows applications that define UNICODE. (RPG Hacker)
-   Labels can now start with underscores. (LDAsuku)
-   An `undef` command inside an unexecuted `if` statement would still get executed. This is now fixed. (randomdude999)
-   The `assert` command would give strange errors when the condition to test contained a comma. This is now fixed. (randomdude999)
-   Trailing commas can now be used to join lines that contain more than one comma, and the joined lines can now have comments. (randomdude999)
-   An invalid warning ID in a `warnings enable` command no longer prints the error message three times. (randomdude999)
-   When using the Asar DLL, built-in defines (e.g. `!assembler_ver`) are now always present. Previously they were missing when the DLL was called without specifying a standard defines file. (randomdude999)
-   Repeatable instructions (like `LSR #4` don't accept labels in their argument anymore. (randomdude999)
-   Asar no longer writes an incorrect checksum for ROMs whose size isn't a power of 2, e.g. 2.5MB, 3MB, 6MB. (randomdude999)
-   The "It would be wise to set the 008000 bit" warning has been relaxed and isn't thrown for labels in banks 70-7F. (randomdude999)
-   Overwriting built-in functions now throws a warning. Overwriting user-defined functions now also throws a warning for consistency. (randomdude999)
-   Opcode length specifications were sometimes being ignored in `arch spc700`. Now `adc.w a, $0` properly assembles to `85 00 00` instead of `84 00`. (randomdude999)
-   Fixed case sensitivity problem in function calls. (p4plus2)
-   Fixed struct definition with label base corrupting label name. (p4plus2)
-   Fixed missing error on certain types of label. (p4plus2)
-   Fix some phantom errors in math processor (randomdude999)
-   Fixed a memory leak in the cli version of asar (p4plus2)
-   Fixed a memory corruption in spc700 processing (p4plus2)
-   Fixed various memory bugs (found via valgrind/fsanitize) (p4plus2)
-   Fixed a case where if statements wouldn't always throw an error (randomdude999)
-   Fixed a case where you could get invisible errors when prefixing commands with `@` (randomdude999)
-   Fixed `arch superfx` ignoring all commands with more than 2 words (randomdude999)
-   Fixed non-ASCII characters in strings being written as garbage (Katrina)
-   Fixed crashes when getting into an infinite recursion loop (randomdude999)
-   Improvements to syntax highlighting in the Asar manual (Atari2.0)
-   The C# wrapper has been updated to work with Asar versions 1.60 and newer. (Vitor Vilela)
-   Improved error message for SPC instructions with out of range arguments (randomdude999)

- - -

## v1.71

**Release:** January 6, 2019

### Contributors:

-   randomdude999

### Bug fixes:

-   The Super FX ROM mapping should now properly support freespace.
-   The `canread()` functions returned true for the first byte after the end of the ROM. That has been fixed.
-   Using `check bankcross off` now should mess with the PC less (i.e. no forced fastrom addressing).
-   Using the `error` command now doesn't print the line of code that caused the error. All warnings except for the one from the `warn` command should now print the line of code that caused the warning. (note that not all warnings or errors are associated with a specific line of code.)

- - -

## v1.70

**Release:** January 4, 2019

### Contributors:

-   randomdude999
-   boldowa

### New features:

-   The `fullsa1rom` mapper now supports automatic freespace searching. (randomdude999)
-   `incbin` ranges can now use math as an alternative to unprefixed hex. To use this, surround the math with parentheses. For example, `incbin file:(4+2)-($F+$10)`. Note that in math statements, unprefixed numbers are decimal, not hex! (randomdude999)

### Bug fixes:

-   Fixed quite a few crashes in Asar:
    -   Fixed a crash when including a directory (randomdude999, found by [Selicre](http://smwc.me/u/Selicre))
    -   Fixed a crash when using a macro sublabel outside of a macro (randomdude999)
    -   Fixed a crash when having unmatched structs in a weird way (randomdude999)
    -   Fixed a crash when appending to a non-existent define (randomdude999, found by Selicre)
    -   Fixed a crash when Asar encountered mismatched quotes after define evaluation (randomdude999, found by Alcaro)
-   Made Asar about 2x faster than before (randomdude999)
-   Values larger than $80000000 in things like `dd` should work better (randomdude999)
-   Fixed a freespace leak when using `prot` (randomdude999, found by WhiteYoshiEgg)
-   Fixed Windows header includes, to make cross-compilation of Asar on Linux work better (boldowa)

- - -

## v1.61

**Release:** August 22, 2018

### Contributors:

-   randomdude999

### Bug fixes:

-   Fixed asar quietly skipping assembling a line if it had a label followed by 3 or more spaces and ended with `"`.
-   In 1.60, `if !condition` was removed without a proper deprecation process. It has been re-added with a deprecation warning, because a few patches still managed to use it.

- - -

## v1.60

**Release:** July 6, 2018

### Contributors:

-   RPG Hacker
-   boldowa/6646
-   zetaPRIME
-   Horrowind
-   randomdude999
-   TheBiob
-   BenJuan26
-   Vitor Vilela
-   KevinCathcart
-   KungFuFurby

### New features:

-   Asar is now officially hosted on GitHub: [https://github.com/RPGHacker/asar](https://github.com/RPGHacker/asar)
-   Asar now uses CMake, which should make it easier to build on different platforms and with different compilers. (RPG Hacker, randomdude999, boldowa)
-   Added a proper manual in HTML format to Asar, rather than just having the xkas manual and a TXT file with all changes. (RPG Hacker)
-   Added Python binding for the Asar DLL. (randomdude999)
-   Added `defined()` function (randomdude999) and `undef` command. (zetaPRIME)
-   Added simple syntax highlighting for Sublime Text. (randomdude999)
-   +/- label declarations can also end with an optional `:` now for consistency. (RPG Hacker)
-   Added macro-local variations for +/- labels and sub labels, which are also prefixed with a `?`. (RPG Hacker)
-   Added macro main label assignment. (RPG Hacker)
    
    ```asar
    macro my_macro()
        ?MacroLabel = $008000
        
        dl ?MacroLabel  ; Valid
    endmacro
    
    %my_macro()
    
    dl ?MacroLabel      ; Error!
    ```
    
-   Added support for a new type of label declaration: prefixing a label or sub label declaration (including macro labels) with `#` will create that label without modifying existing label hierarchies. This is mainly intended for hacky incsrc/routine macros as used by GPS, which can break existing label hierarchies when used. (RPG Hacker)
    
    ```asar
    macro my_new_routine()
        jsl MyNewRoutine
                
        !macro_routine_defined ?= 0
        
        if !macro_routine_defined == 0
            pushpc
            
            freecode cleaned
            
            #MyNewRoutine:
                incsrc routines/mynewroutine.asm
            
            pullpc
        
            !macro_routine_defined = 1
        endif
    endmacro
    
    Main:
        %my_new_routine()
    .Sub
    
        ; Both of these are found
        dl MyNewRoutine
        dl Main_Sub
    ```
    
    Note that while it's technically possible to use this feature on sub labels, macro labels and macro sub labels, I don't think there's any reasonable use case for this. In most cases, regular macro labels and regular macro sub labels are recommended for usage inside macros.
-   Added support for include search paths. (RPG Hacker)
-   Added asar\_patch\_ex() function to DLL API, which works like asar\_patch(), but takes additional parameters. (RPG Hacker)
-   You can now escape `!` by doing `\!` (useful for preventing Asar to replace defines in certain places). (randomdude999)
-   You can now escape `\` by doing `\\`. (RPG Hacker)
-   Added support for `pushbase`/`pullbase`. (randomdude999, RPG Hacker)
-   Added `check title` command which makes Asar verify the title of the output ROM (to assure a patch is applied to the correct ROM). (randomdude999)
-   Added support for nested namespaces. (randomdude999, RPG Hacker)  
    Enable them via the command:
    
    ```asar
    namespace nested on
    ```
    
-   Added support for `check bankcross off` which disables checking for bank crossing. Use with caution; might not work with certain features or outright break some stuff. (randomdude999, RPG Hacker)
-   Added a new, simple README. (randomdude999, RPG Hacker)
-   Added a `filesize()` function and a `getfilestatus()` function. (randomdude999)
-   Added support for `stdincludes.txt` and `stddefines.txt` - see manual for details. (RPG Hacker, randomdude999)
-   Added support for setting defines via the command line or DLL API. See manual (section: usage) for details. (randomdude999)
-   Added `includeonce` command for shared files which may be included multiple times, but should only be assembled once. (RPG Hacker)
-   Added support for generating a symbols file. Currently, WLA and no$sns format are supported. (KevinCathcart)
-   Opcode length specifiers (`.b`, `.w` and `.l`) are now also supported for the SPC700 architecture. (KungFuFurby)
-   Added `stringsequal()` and `stringsequalnocase()` functions. (RPG Hacker)
-   Added support for some built-in defines. Currently, `!assembler` and `!assembler_ver` are supported. Trying to modify those defines will throw an error. (RPG Hacker)
-   Added IDs to all warnings and errors, together with functionality to enable or disable specific warnings. (RPG Hacker)
-   Added a new optional warning that has to be enabled manually to throw warnings when opcodes are implicitly sized. (KevinCathcart)
-   Added functionality to allow overriding Asar's default behavior of enabling/disabling checksum generation based on context. (randomdude999)

### Bug fixes:

-   Fixed various bugs in the DLL interface. It should now hopefully be possible to apply multiple patches in succession via the DLL interface without resorting to hacks. (RPG Hacker, randomdude999, boldowa)
-   Rewrote big parts of test applications once again to make it a lot easier to test a bunch of ASM files and actually get some meaningful information out of it. (RPG Hacker)
-   Fixed the way `;@` works. This command was really meant for backwards-compatibility with xkas and is supposed to assemble everything following it, which it now does again. (RPG Hacker)  
    As a result of this change, any of the following are now valid Asar code:
    
    ```asar
    ;@asar 1.60
    @asar 1.60
    asar 1.60
    ```
    
    When Asar finds an unknown command on a line starting with `;@` or `@`, it only throws a warning instead of an error. You can make use of this fact by using optional features from newer Asar versions and still have your patch assemble in old Asar versions, where those features are ignored (don't know how practicable and useful this really is, but in theory, it should be possible). And of course it can be used for patches that are compatible with both xkas and Asar, not that that's particularly useful anymore in this day and age.
-   Changed format of existing command line arguments for consistency. (RPG Hacker)
-   LoROM banks `$70` to `$7D` can be used correctly. (randomdude999)
-   Struct definitions are cleared correctly in Asar lib. (randomdude999)
-   Fixed a freeze that could occur when using `freedata align`. (TheBiob)
-   Fixed a bug in `pad` command that made it not update the pc correctly and also made it trigger a bank cross error before actually writing any data into a new bank. (RPG Hacker)
-   Fixed a rare crash when using the command-line interface in ways you're not supposed to use it. (randomdude999)
-   Fixed `readfile()`/`canreadfile()` crashing when reading from more than 16 different files in the same patch. (randomdude999)
-   Fixed the ROM title being reported incorrectly. (randomdude999)
-   Fixed using really large values in math with `math round on` making the values negative sometimes. (randomdude999)
-   Fixed an SA-1 mapping issue. (Vitor Vilela, randomdude999)
-   Fixed `print double()` not working when any of their arguments contain commas or parentheses. (randomdude999)
-   Made it possible to pass string parameters to functions like `readfile1()` via user-defined functions. Previously, this failed as Asar expected all parameters in user-defined functions to be numbers. (KevinCathcart)
-   Fixed a bug related to evaluation of parameters in user-defined functions. In some situations, Asar returned the incorrect parameter. For example: given a user-defined function with a parameter `ab` and a paramater `abc`, Asar occasionally returned the parameter `abc` when referencing the paramter `ab`. (KevinCathcart)
-   Added more undocumented features to the manual. (RPG Hacker)
-   0x hex literals were supported by Asar on some platforms, which wasn't supposed to be the case. Made sure 0x hex literals throw errors on all platforms. (randomdude999)

### Deprecated features:

-   Started the process of deprecating xkas compatibility mode. From now on, using xkas compatibility features will throw warnings. In a future version of Asar, xkas compatibility will be removed entirely.
-   Started deprecating `autoclear`, which is an alias of `autoclean`. Please use only `autoclean` from now on.
-   Started deprecating `freespace fixed`, which is an alias of `freespace static`. Please use only `freespace static` from now on.
-   Deprecated and removed support for `if !condition`. This feature was planned for deprecation as it bites with Asar's define syntax, but it was removed earlier than planned because, after inspecting the code, it was determined that it didn't work properly and probably wasn't even usable in versions prior to 1.60 at all. The only way to use it in Asar 1.60 was by using the new escape sequence for !, which didn't exist in earlier Asar versions. Thus it can be assumed that the feature wasn't used in previous Asar versions and can be removed safely. To negate conditions in Asar 1.60, either use the built-in logic functions (`if !(a == 0 && b == 0)` becomes `if not(and(equal(a, 0),equal(b, 0)))`) or directly negate the condition (`if !(a == 0 && b == 0)` becomes `if a != 0 || b != 0`).

- - -

## v1.50

**Release:** February 28, 2017

### Contributors:

-   RPG Hacker
-   p4plus2

### New features:

-   Added support for structs.
-   Added API to Asar lib for getting information on all blocks of data written by Asar.
-   Added API to Asar lib for getting the mapper currently used by Asar.
-   Added support for ExLoROM and ExHiROM mappers.  
    NOTE: Based entirely on conversion tables I got from Ladida; don't know if these conversions are actually correct. Some features may not work as intended when using those mappers (such as `freedata`, for example), but I can't verify this.
-   Added `pushtable` and `pulltable` commands, which let you back-up or restore the current character table to the/from the stack.
-   Added `ext\notepad-plus-plus\syntax-highlighting.xml`. This file can be imported into Notepad++ as a user-defined language to add syntax highlighting for Asar patches, featuring all commands currently supported by Asar. By default, this syntax highlighting is enabled on all files with an extension of `.asm` or `.asr`, but this can be customized via Notepad++.

### Bug fixes:

-   Lines starting with `@` or `;@` that don't map to a recognized special command now only throw warnings at best and no errors.
-   Rewrote code of tests a little to make them easier to execute and make them clean up their own mess.
-   C# wrapper for Asar DLL was non-functional since it didn't specify a calling convention, making it always lead to an exception in some scenarios.

### Notes:

-   Just like the last version, this version of Asar was built in MSVC rather than g++, but this time I also updated the Asar DLL (which I had overlooked last time). I'm not sure if Windows applications are compatible with DLLs that were built by a different compiler, so if you're planning to use the DLL, this is something to watch out for. If you're planning to use a compiler other than MSVC, I recommend just rebuilding the DLL from source in whatever compiler you're using (or directly including the Asar library code in your project).

- - -

## v1.40

**Release:** October 23, 2016

### Contributors:

-   RPG Hacker

### New features:

-   `readfile` functions: `readfile1(filename, offset)`, `readfile2(filename, offset)`, `readfile3(filename, offset)`, `readfile4(filename, offset)` - similiar to `read1()` etc. functions, except data is read from another file instead of from the ROM (note that offset is a PC offset, not a SNES offset). You can pass an optional third value which is returned if the read fails. These functions are primarily intended for reading bytes from another file and then doing math with them. For example: reading bytes from a Lunar Magic .pal file, converting them into a different format and then writing them to the ROM as a table that can directly be DMA'd to CGRAM without further conversions (all conversions happen at compile-time). As an additional bonus, all of those functions cache any file passed to them (up to 16 simultanous files), which means that multiple readfile() calls on the same file will keep the file open rather than repeatedly opening and closing the file.
-   `canreadfile` functions: `canreadfile1(filename, offset)`, `canreadfile2(filename, offset)`, `canreadfile3(filename, offset)`, `canreadfile4(filename, offset)`, `canreadfile(filename, offset, length)` - basically the `readfile()` equivalents of `canread1()` etc.
-   `snestopc(address)` and `pctosnes(address)` functions: for manually converting addresses (note that those functions are dependent on the ROM's current mapping mode, so use them with caution - chances are you will never need them at all).
-   `max(a, b)`, `min(a, b)` and `clamp(value, min, max)` functions: `max()`/`min()` return the maximum/minimum of two values, whereas `clamp()` makes sure that that value is `>= min` and `<= max.`
-   `safediv(dividend, divisor, exception)` function: divides `dividend` by `divisor`, unless `divisor` is 0, in which case `exception` is returned.
-   `select(statement, true, false)` function: if `statement` is 0, `false` is returned, in any other case, `true` is returned. Basically, a mathematical version of "if/else". Please note that unlike if/else blocks, function arguments in Asar are always evaluated before a function returns. In other words: if you do `select(1, 1/1, 1/0)`, Asar will throw a "division by zero" error, even though the function would return 1/1. In this particular case, it's recommended to simply use the `safediv()` function in place of a regular division.
-   `not(value)` function: returns 1 if `value` is 0 and 0 in any other case.
-   comparison functions: `equal(a, b)`, `notequal(a, b)`, `less(a, b)`, `lessequal(a, b)`, `greater(a, b)`, `greaterequal(a, b)` - rather self-explanatory, return 1 if the respective comparison is true and 0 otherwise. Primarily intended to be passed as statement to `select()` function.
-   logical functions: `and(a, b)`, `or(a, b)`, `nand(a, b)`, `nor(a, b)`, `xor(a, b)` - also self-explanatory, return 1 if the respective logical operation is true and 0 otherwise. Primarily intended to be passed as statement to `select()` function.
-   `while` loops: Added compile-time `while` loops to Asar. Those work similar to if conditionals, with the difference that their blocks are assembled repeatedly until their condition becomes false. For easier implementation and higher compatibility, while loops are terminated with `endif`, just like `if` conditionals. When using while loops, be careful not to cause an infinite loop. Asar won't make any effort to detect those.
-   Multiline operator: You can now put `\` at the end of any line of source code and Asar will append the next line to it. This is similar to putting a `,` at the end of a line, with the difference, that the `\` itself does not appear in the concatenated string, whereas the `,` would. This is useful to split long function definitions into multiplie lines, for example. Note that all whitespace following the `\` is ignored, whereas whitespace preceeding the `\` isn't. Therefore
    
    ```asar
    db\
    $FF
    ```
    
    turns into
    
    ```asar
    db$FF
    ```
    
    for example, whereas
    
    ```asar
    db \
    $FF
    ```
    
    turns into
    
    ```asar
    db $FF
    ```
    
-   `double(num)` print function: Can be passed to print to print a double variable with its fractional part. Has a default precision of 5 decimal places, but can be passed an optional second argument to override the precision.
-   `round(num, precision)` function: Rounds the double variable `num` to `precision` decimal places. Pass 0 as precision to round to the nearest integer.

### Bug fixes:

-   Asar 1.37 officially suppported overloaded versions of `read1()` to `read4()`, but always threw "Wrong number of parameters to function" errors when actually using those overloaded versions.
-   Asar 1.37 threw "Wrong number of parameters to function" error for function `canread()` when passing 2 arguments to it, because it actually treated it as `canread1()` due to an internal error in string comparison.
-   Using better double -> int conversions in some places - where
    
    ```asar
    dd $FFFFFFFF
    ```
    
    would assemble to `00 00 00 80` (`$80000000`) in Asar 1.37, it now assembles to `FF FF FF FF`
-   Defines in `elseif` conditionals now get properly resolved.
-   The `#=` define operator now doesn't truncate its value when using `math round off`, making it possible to do double-precision math with it.
-   Asar 1.37 detected misplaced elses and endifs, but not misplaced elseifs.
-   Putting `@xkas : @asar 1.37` on the first line would previously lead to an error, whereas putting `@asar 1.37 : @xkas` there would not. Both variations lead to an error message now, since it really doesn't make much sense to use them together in any combination.
-   Special commands like `@asar` or `@include` could previously be used on the first line only and needed to be chained with a `:` inbetween. They can now be used on any line as long as no other command comes before or inbetween them.
-   Asar 1.37 fixed a bug in SuperFX compilation, but `src/test/arch-superfx.asm` was never edited to acknowledge this fix, so the test always failed
-   Added different define operators (`=`, `+=`, `:=`, `#=`, `?=`) to manual.txt. Those have been in Asar for quite a while, but were never documented yet, although they can be quite useful.

### Notes:

-   This version of Asar was built in MSVC rather than g++, mainly because I already had that installed and use Visual Studo as an IDE, anyways. Functionally, this shouldn't make any difference, unless using Asar in unintended ways, where anything goes. I did build the Linux version in g++ to confirm compatibility, though.

- - -

## v1.37

**Release:** March 26, 2016

### Contributors:

-   Raidenthequick

### New features:

-   New `freespace` argument added; a `$xx` byte that will search the ROM for contiguous sections of that byte. Before it was hardcoded to only search for `$00`. Default is still `$00` if not supplied, so past patches should not be broken.
-   In line with this, `autoclean` was hardcoded to clean using `$00`. This was fixed also to clean with the byte supplied by freecode, or `$00` if not supplied.

### Bug fixes:

-   Super FX short addressing fixed, and added error checking for valid short address. For example, `lms r0,($00D4)` used to output `3D A0 D4`, which is actually incorrect because short addressing doubles the byte supplied by the instruction to give a range from `$0000`\-`$01FE` with just one byte (since Super FX reads words). This now outputs `3D A0 6A` which is correct. Also, asar now throws an error for anything outside `$0000`\-`$01FE` as well as all odd-numbered addresses for both `sms` and `lms` instructions. (Odd-numbered addresses cannot be accessed using short addressing due to the multiplying by 2.)
-   Super FX and SPC700 labels were broken if used within freespace. This has been fixed by doing what 65816 does: mask the address with `0xFFFFFF` because freespace addresses use a high byte to indicate that they're freespace.
-   Fixed SA-1 mapping using wrong Super MMC banks range.
