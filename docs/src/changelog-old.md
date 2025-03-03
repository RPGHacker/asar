# Historical changelog

This part of the changelog was written retroactively and may be incorrect and/or incomplete.

## v1.36

**Release:** November 28, 2013

### Contributors:

-   Alcaro

### New features:

-   Added the fullsa1rom mapper.
-   Asar now halts if more than 20 errors are detected.

### Bug fixes:

-   Fixed a warning about $xx,y not existing if $xxxx,y doesn't exist either
-   Fixed sublabels requiring a parent label even in xkas compatibility mode
-   Fixed a memory leak in the DLL

- - -

## v1.35

**Release:** December 13, 2012

### Contributors:

-   Alcaro

### New features:

-   Made freecode print the size of the requested freespace if allocation fails.

### Bug fixes:

-   Fixed a crash caused by incbin'ing a file and specifying an endpoint beyond the end of the file.
-   Blocked readN on exactly one byte after the end of the input ROM.

### Deprecated features:

-   fastrom has been removed. It gives far too many problems relative to its advantages (are there any?). In exchange, freespace searching in lorom is now always fastrom enabled. (The command itself still exists, but it does nothing.

- - -

## v?.??

**Release:** November 11, 2012

### Contributors:

-   Alcaro

### New features:

-   Added -DTIMELIMIT support on Windows. Note that it's more limited than on Linux; it seems to only check the time limit every seven seconds, it can't print error messages, and it bugs up on anything above 429 seconds. (But you shouldn't need to set it above one or two anyways.)

### Bug fixes:

-   Missing an org/freespace command now no longer places the first byte at SNES $FFFFFF, but jumps to $008000 directly.
-   Allowed JMP.l in xkas mode...
-   Fixed PC->SNES conversion for 0x380000 and higher.
-   Fixed BEQ a.
-   fastrom : org $8000 : db $01 : warnpc $8001 no longer throws errors.
-   Made bit shifting unsigned, to match how xkas behaves.
-   Made db $0-10 return -10, not -16.
-   Including a file that enables xkas mode, while xkas mode is already enabled, no longer throws errors.
-   Fixed dd $FFFFFFFF acting like dd $7FFFFFFF.
-   Fixed compile errors on GCC 4.7.
-   Made blockreleasedebug and blockdebugrelease not show up anymore.
-   Files saying @asar can no longer be included from files saying ;@xkas.
-   Locked org $F00000.

- - -

## v1.33b

**Release:** October 26, 2012

### Contributors:

-   Alcaro

### Bug fixes:

-   Rearranged the manual a little to have a more obvious filename.
-   Nuked a debug code that kept printing nonsense.

### Notes

-   Asar now considers OS X a non-Linux Unix-line and recommends -Dlinux.

- - -

## v1.33

**Release:** October 22, 2012

### Contributors:

-   Alcaro

### New features:

-   Made incbin file.bin -> $123456 possible. This one can go as far as you want; it doesn't mind crossing banks nor exceeding the size of a RATS tag. It does, however, demand to know the SNES position in advance; you can't use labels here. (It also refuses to go outside the ROM)
-   Added a sandbox mode that disables everything that loads external files. Only accessible by recompiling (-DSANDBOX) since it should only be useful for people who have compilers already.
-   Added a way to make the process abort if it runs too long. Linux+recompile (-DTIMELIMIT=60) only.

### Bug fixes:

-   Nuked some spurious warnings in incsrc/incbin if the base path contains backslashes.
-   Made Asar a little more eager to detect bank crossings. Now those errors appear in the correct file.
-   Made REP #$FFFF throw errors instead of repeating 65535 times.
-   Fixed a bunch of weird code in the freespace finders.
-   Made sure . does not count as a label.
-   Fixed LDA Label,y in emulation mode. Asar tried using 24-bit addressing for that, but there is no such addressing mode.
-   Another accuracy improvement in emulation mode.
-   Detonated some more SPC700 inaccuracies and bugs.
-   Made JMP ($1234)+($3412) assemble to JMP abs, not JMP (abs).

- - -

## v1.32

**Release:** October 8, 2012

### Contributors:

-   Alcaro

### New features:

-   Added @xkas, which will revert to maximum xkas compatibility mode and change the behaviour of a couple of commands (as well as throwing warnings on various Asar-only commands).
-   Any line starting with ;@ will now be assembled. This is intended to be combined with the above one to create ;@xkas, which will send Asar into xkas mode while letting xkas ignore it, but you can also use ;@if 0 : org !Freespace : ;@endif : ;@freecode (except with the colons replaced with linebreaks).
-   Replaced bank off with bank auto, and added bank noassume, which always uses long addressing if possible.
-   Removed the @ trick for suppressing warnings. Its only legit use was telling Asar that a freespace is cleaned in a way it can't understand. freecode cleaned is the new way to say that.
-   Whitespace, blank lines and comments are now allowed to exist before @asar, ;@xkas and @include/@includefrom.
-   Added !define ?= value, which sets !define to value if !define is previously undefined.

### Bug fixes:

-   Blocked \\ in incsrc/bin because linux.
-   Reworked prot a bit. You can now use multiple prot statements in the same freespace; Asar has always allowed cleaning them out anyways. The old implementation was rather hacky.
-   Fixed build scripts a little. Some files didn't get included.
-   Fixed the internal ROM size when expanding SA-1 ROMs.
-   Killed some dead code.
-   Made db 0x80 assemble properly with math round on (the default).
-   Killed a phantom error on ALS A and a couple of similar constructions.
-   Squished a possible moving labels error.
-   Fixed a bug where the major version of Asar got compared backwards in @asar.

- - -

## v1.31

**Release:** September 13, 2012

### Contributors:

-   Alcaro

### New features:

-   Added incbin file.bin -> Labelname support.
-   Added align detail to freespace/freecode/freedata.
-   Made reset freespaceuse a valid operation.
-   Added arch spc700.

### Bug fixes:

-   Killed some more Linux compile errors.

### Deprecated features:

-   Deprecated arch spc700-raw.

- - -

## v1.30

**Release:** August 24, 2012

### Contributors:

-   Alcaro

### New features:

-   Added SuperFX mapper and arch superfx.
-   Unlocked $\[F0-FF\]:\[8000-FFFF\] in lorom again. Turns out bsnes only maps SRAM to the lower halves of those banks.
-   Added an elseif command.
-   Added && and || support to if/etc conditionals.
-   Reworked the Relative branch out of bounds errors a little, so it tells how far outside the bounds they are.
-   Added print dec($1234) and print hex(1234).
-   Attached test suite in the source zip, in case some other programmers are interested.
-   Added @include and @includefrom, which throws errors if Asar is invoked directly on them.
-   Added a -pause=<action> flag on the command line. Valid actions are "yes", "warn", "err", and "no" (default).
-   Added a couple of synonyms to the single-bit SPC700 opcodes.
-   Added underscore-prefixed versions of all builtin functions.
-   Added error-safe readN and canreadN.

### Bug fixes:

-   Reworded the 125 freespaces limit to make sure nobody can think there's a limit of 250 labels.
-   Made readN()ing and writing to the same byte in ROM well defined (it gives the same value as it had in the input ROM).
-   Made the size guesser a little smarter. It now assumes .b for LDA #$123456/65536.
-   Killed a phantom error on the very rare code pattern db $80/(Label2-Label1) : Label1: : db 0 : Label2: and similar.
-   Adjusted the error messages read1/etc gives if the address is invalid.
-   Suppressed a couple of useless errors if labels are redefined.
-   Killed a couple of Invalid endif errors if an if statement is invalid.
-   Repaired nested pushpc/freedata.

- - -

## v1.29

**Release:** July 19, 2012

### Contributors:

-   Alcaro

### New features:

-   Added -v and -version switches.
-   Removed the lock on $700000-$7DFFFF in HiROM as it's not mapped to SRAM there.
-   Edited the library API a bit.

### Bug fixes:

-   Added an error message for mov (x)+,a in the SPC700 modes.
-   Nuked a bug making tabs not treated as spaces in all contexts.
-   Fixed a bug where Label = 123 checked that there is an org in front.
-   Fixed a bug where the line number was removed if the last code block on a line crossed a bank border.

- - -

## v1.28

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Flipped the switch to autocorrect the checksum to on by default.

### Bug fixes:

-   Nuked some double spaces in outputs.
-   Fixed sa1rom mapping breaking on everything except the first megabyte.
-   Fixed a couple of bugs with valid table files being rejected.

- - -

## v1.27

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Made freespace fixed a synonym of freespace static.
-   Made Asar try to autodetect whether the ROM is LoROM or HiROM. Homebrews default to LoROM.

### Bug fixes:

-   Zapped a phantom error if the code arch spc700-raw : org $010000 : db $00 is used.
-   Nuked some pointless errors with static freespaces.
-   Zapped a bunch of bugs with static/fixed freespace.
-   Killed a bug that crashed Asar if a line contained both commas and unmatched quotes.
-   Replaced the Press Enter to continue with the standard Press any key to continue prompt on Windows.
-   Refactored some parts of the math parser, to get rid of some possible escaping NaNs.
-   Added some #errors if no frontend is chosen, and edited some others a bit.
-   Adjusted Linux support a bit. Looks like it bitrotted.
-   Fixed some off-by-one errors in spc700-inline.

- - -

## v1.26

**Release:** April 28, 2012 or earlier

### Contributors:

-   Alcaro

### New features:

-   Added support for db +1. There's no reason not to allow it.

### Bug fixes:

-   Removed some more files made useless by dropping asar-clr.dll.
-   Fixed a bug where db $12,$34 broke in the spc700 modes.
-   Fixed a bug where an invisible bank border was hit if a base command was used without an org command.

- - -

## v1.25

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Rewrote the .NET library as a C# wrapper around asar.dll using P/Invokes, for better Linux compatibility (Mono really seems to hate C++/CLI). This also means that asar-clr.dll no longer exists.
-   Made errors not shred the ROM in the library frontend.

### Bug fixes:

-   Killed a bunch of moving label errors if a PROT is misplaced. No point throwing 300 errors for one misplaced line.
-   Fixed error messages erroneously being printed to stdout instead of stderr.
-   Fixed errors with prot being incompatible with fastrom.
-   Fixed an uninitialized variable.
-   Fixed a buffer underflow Valgrind whined about.
-   Zapped a null pointer bug in the library if a file was not found.
-   Nuked a bug where the library did not clear out previous errors when applying a new patch.

- - -

## v1.24

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Added SA-1 support. Syntax is sa1rom 0,1,2,3, where the four numbers are the bank numbers, 0 through 7. It's allowed to not give them; that will result in 0,1,2,3 being picked as default.
-   Added freespace finding support for hirom and sa1rom.
-   Made Asar remove extranous quotes around filenames if a file is pulled to the Asar window.
-   Minor adjustment to the ROM title verifier.

### Bug fixes:

-   Fixed a phantom error if prot isn't used directly after a free\* statement.
-   Fixed a bogus error if prot is placed at any of the first eight bytes of a bank.
-   Repaired autoclean, which appears to have broken a few versions ago.
-   Fixed a crash bug if the ROM does not exist and couldn't be created.

- - -

## v1.23

**Release:** ?

### Contributors:

-   Alcaro

### Bug fixes:

-   Fixed around ten broken SPC700 opcodes.

- - -

## v1.22

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Added a few special cases for some functions, and adjusted some compiler flags. The result is dramatically improved performance.
-   Adjusted the string table a bit.

### Bug fixes:

-   Fixed an uninitialized value in the macro handling code that may (but usually doesn't) throw errors if a macro has no arguments.
-   Repaired partial incbin if the filename is quoted.

- - -

## v1.21

**Release:** ?

### Contributors:

-   Alcaro

### Bug fixes:

-   Fixed: read1() etc. went missing.

- - -

## v1.20b (?)

**Release:** March 4, 2012

### Contributors:

-   Alcaro

### Bug fixes:

-   Added freecode static and norom to changes.txt.

- - -

## v1.20 (?)

**Release:** March 4, 2012

### Contributors:

-   Alcaro

### New features:

-   freecode/freedata have been expanded: They now take an optional parameter containing comma separated parameters on how to hand out the freespace. Valid parameters are ram, noram, and static. ram tells Asar that this code needs the RAM mirrors and puts the code in banks 10-3F; noram tells Asar that the code (or table) does not need the RAM mirrors, and therefore uses banks 40-6F if possible; static demands that the freespace does not move; if it grows, Asar throws an error. freespace has also been added; freecode is a shortcut to freespace ram.
-   Added hirom support. It works just like in xkas: hirom : org $C00000 : JML Main. Also added norom, where the PC address is the same as the SNES address (not counting SMC headers); use this (and base) if you want to implement your own mapper (preferably in a macro).
-   Simplified table code, and made it accept assigning stuff to the = character.
-   LDY #Label>>16 (and LDA/LDX/etc) is now considered 8bit. This should reduce the need for explicit .b hints.
-   Asar can now use the last byte of the ROM as freespace.
-   Made the default value for untouched table bytes be more random to make errors in usage more obvious.
-   Lowered the recursion limit a bit.
-   You can now drop the extensions on both filenames, if you want to.
-   Asar now tries to no longer parse defines inside if 0 blocks. Note that this doesn't help on single-line if statements; you'll need a multiline if.
-   Added !def := val, which expands everything inside the value before setting it (making !a := a!a a perfectly safe operation).
-   Added !def #= val, which expands the value into an integer and stores it into the define. !a #= 1+1 : L!a: is a valid operation.

### Bug fixes:

-   Fixed fastrom again again again.
-   Labels before the first org/freecode/etc are now considered to be at $008000, not $FFFFFFFF. (They're still considered invalid.)
-   Some minor fixes in autoclean dl Label.
-   Repaired the \*\* operator.
-   Calling a function no longer hits a user-defined function starting with the name of called function.
-   Fixed skip command, which previously only edited the base address.

- - -

## v1.14 (?)

**Release:** February 8, 2012

### Contributors:

-   Alcaro

### Bug fixes:

-   Killed a crash bug.

- - -

## v1.13 (?)

**Release:** February 7, 2012 (?)

### Contributors:

-   Alcaro

### New features:

-   Added a random icon, to fit the Nordic theme of Asar's name.
-   Made asar\_patch destroy the ROM if any errors are encountered, to ensure that nobody attempts to use a ROM that may contain subtle breakage.
-   A label can now be on the same line as a macro call.

### Bug fixes:

-   Fixed the bug that makes macros look like they're called from themselves if the macro call is broken.

- - -

## v1.12 (?)

**Release:** February 6, 2012 or earlier

### Contributors:

-   Alcaro

### Bug fixes:

-   Fixed fastrom mode again...
-   Fixed crashes when closing Asar while waiting for input.
-   Macro definitions are now ignored inside if 0 blocks.
-   Macros and incsrc may now be called inside if 1 blocks.
-   Locked banks $F0-$FF from usage. They are not usable.

### Deprecated features:

-   Removed most of the emulation mode. The only remaining change is that it makes Asar print the errors to stdout instead of stderr, since that quirk isn't fixable by editing the ASM files.

- - -

## v1.11 (?)

**Release:** January 25, 2012 or earlier

### Contributors:

-   Alcaro

### New features:

-   Prefixing a command with @ will now make Asar not print warnings for that line.

### Bug fixes:

-   JMP and JSR within the current bank with bank active has been fixed.
-   xkas appears to allow sublabels starting with numbers. Asar has been updated to do the same.
-   fastrom no longer messes up branches.
-   autoclean read3($123456) can now be used before the first org.
-   db ($04<<2)|($80>>1) no longer breaks due to unmatched parentheses.
-   Fixed problem with autoclean $123456, where it read a pointer from $123456 instead of removing $123456.
-   Fixed a crash if a freespace block protects itself.

- - -

## v1.10 (?)

**Release:** January 13, 2012

### Contributors:

-   Alcaro

### New features:

-   Made asardll.h and asardll.cpp work if compiled as both C and C++, and renamed asardll.cpp to asardll.c, for greater compatibility with other languages (hi, Objective-C). As an effect of this, the library name argument has disappeared from asar\_init.
-   base off is now a valid operation. It tosses the base address back to the current code insertion location.
-   New command: bank. It makes the label optimizer act as if it's in another bank. This is not the same as base; LDA Label,x : Label: db $01,$02,$03,$04 will use 24-bit addressing for the LDA if bank is active. It's intended for long codes that assume the data bank register is not the same as the code bank register.
-   Added a copy of GPLv3 to asar.zip, since LGPL is apparently meaningless on its own. These licenses are starting to get a little tricky.

### Bug fixes:

-   Reworded the documentation for prot.
-   Asar now throws a warning if a freespace block appears to be leaked.
-   Adjusted "autoclean at the end of a RATS tag" error a bit.
-   Made Asar not read uninitialized/garbage memory if there's crappy content at the end of a macro.
-   Fixed infinite loop if a freespace of size 32768 is requested.

- - -

## v1.09 (?)

**Release:** January 6, 2012

### Contributors:

-   Alcaro

### New features:

-   New command: warn.
-   The math parser now accepts strings like 0.5 if you turn off rounding. It'll break if rounding is on, since that wouldn't make any sense. It'll round down as soon as it's returned from the math parser; db 0.9 is the same as db 0, but db 0.4\*0.4 is the same as db 1.
-   assert can now take another parameter. If this is given, it is printed in the error message. (All error-generating blocks are printed, which makes these messages appear twice)
-   Asar now attempts to check if the ROM title looks sane before applying a patch.
-   Added Linux/OSX support to asardll.cpp.
-   Added asar\_math() to the library frontend.
-   Now prints a message if patching is successful. However, calling it from the command line makes it remain silent, unless you add -verbose.

### Bug fixes:

-   Restored documentation of the functions to changes.txt that were forgotten when removing stdlib.asm.
-   Fixed crashes if a macro is called with wrong number of arguments.
-   Fixed silent errors when using LDA Label,y if Label is in another bank.

- - -

## v1.08 (?)

**Release:** December 27, 2011

### Contributors:

-   Alcaro

### New features:

-   Added special meaning to the define !assembler: Its value is always "asar", even if you assign something else to it. Intended usage: !assembler = xkas : %freespace\_!assembler(), where %freespace\_xkas() requires the user to set freespace and %freespace\_asar() contains a freecode.

### Bug fixes:

-   Restored db 1+'a' support to the math parser.
-   Merged the dupe descriptions for the fastrom command in the documentation.
-   Fixed sublabel support, which the new math engine broke as well.

- - -

## v1.07 (?)

**Release:** December 25, 2011

### Contributors:

-   Alcaro

### New features:

-   Replaced the math library with a more powerful one.
-   New command: math.
-   Homemade functions can now safely replace builtin ones, to compensate for the possibility that new builtin functions may collide with ones in your codes.
-   Added a few new functions: log, log10, and log2.

### Bug fixes:

-   Removed #ifdef ALCAROBOT (sandbox mode) from a few source files.
-   Fixed a few more memory leaks.

- - -

## v1.06 (?)

**Release:** December 25, 2011

### Contributors:

-   Alcaro

### Bug fixes:

-   Sprite Tool compatibility was still broken. Looks like tmpasm.bin is unheadered after all. Repaired again.

- - -

## v1.05 (?)

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Updated libsmw and libstr to the latest versions from AlcaRobot.

### Bug fixes:

-   Asar no longer crashes if it tries to open a file of less than 512 bytes if it thinks it's headered. This fixes Sprite Tool compatibility.

- - -

## v1.04 (?)

**Release:** ?

### Contributors:

-   Alcaro

### Bug fixes:

-   $xxFFFF can now safely be overwritten.
-   Removed garbage from some errors.

- - -

## v1.03 (?)

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Renamed the DLL frontend to LIB, since DLLs only exist on Windows and dynamic libraries exist elsewhere too. No point using more Windows terminology than needed.
-   Various edits to the library frontends. asar\_i\_\* has been renamed to asar\_\*, and errors and warnings now tell where they're called from, if found inside a macro.
-   The command line frontend tells where problematic macros are called from.
-   Made license choice explicit.

- - -

## v1.02 (?)

**Release:** ?

### Contributors:

-   Alcaro

### New features:

-   Made it possible to call asar\_i\_patch() multiple times without calling asar\_reset() between them. The errors will remain until you asar\_reset() is used, meaning they'll accumulate until they are cleared.

### Bug fixes:

-   Fixed a crash bug in asar\_i\_reset() if the function does not create any custom functions.

- - -

## v1.01 (?)

**Release:** ?

### Contributors:

-   Alcaro

### Bug fixes:

-   Put a few lines back where they should be, to get rid of a crash.

- - -

## v1.00 (?)

**Release:** December 2, 2011

### Contributors:

-   Alcaro

### New features:

-   If an error occurs, it now prints the buggy block, if relevant.
-   Various changes to the DLL API.
-   Crappily cobbled together a hack to make Asar compatible with Sprite Tool (a file called tmpasm.bin is now considered to be headered).
-   Included the script used to compile the .NET DLL.

### Bug fixes:

-   Fixed a memory allocation mismatch.

### Deprecated features:

-   Dropped autocolon, since it's useless and never used.
-   The set command has been removed.
-   stdlib.asm has lost its special meaning.

- - -

## v?.??

**Release:** November 29, 2011

### Contributors:

-   Alcaro

### New features:

-   asar-cli.dll has been renamed to asar-clr.dll, and its contents has been moved to namespace AsarCLR instead of asarcli. This will require your tools to be edited, but it shouldn't take longer than a minute or two.
-   Added a function to the DLLs to view the table data.
-   New setting: werror. It makes warnings emit errors if encountered.

### Bug fixes:

-   pushpc no longer throws idiotic errors everywhere.
-   asar-clr.dll can now return errors/warnings/etc without crashes.
-   Killed off a bunch of memory leaks in asar-clr.dll.
-   STA $12,y warnings now print only once.
-   asar\_resolvedefines no longer throws exceptions outside the DLL.
-   Squashed a memory leak bug where the math module allocates some memory without freeing it when the DLL is unloaded.

### Deprecated features:

-   Removed AsarCLR::Asar::unmanageerrors() from asar-clr.dll. It's an internally used function that shouldn't be exported.

- - -

## v?.??

**Release:** November 21, 2011

### Contributors:

-   Alcaro

### Bug fixes:

-   Fixed a bug that made + and - labels not be treated as labels under some circumstances.
-   Made "relative branch out of bounds" errors disappear again unless they're supposed to exist.

- - -

## v?.??

**Release:** November 20, 2011

### New features:

-   if 0 now blocks label creation.
-   asar-cli.dll now exports a set of functions using .NET types instead of C types.
-   The DLL APIs have been slightly changed with regards to initialization and stdlib.asm.
-   Asar now prints warnings and errors to stderr instead of stdout. (Emulation mode is unaffected.)

### Bug fixes:

-   Repaired "branch out of bounds" errors.

- - -

## v?.??

**Release:** November 17, 2011

### Contributors:

-   Alcaro

### Bug fixes:

-   Removed more debug code
-   Fixed org Label if the label is in freespace

- - -

## v?.??

**Release:** November 17, 2011

### Contributors:

-   Alcaro

### New features:

-   You can now use if !condition to negate the statement. Note that you may need an extra set of parentheses due to conflicts with define syntax.
-   Parentheses can now be used inside macro arguments. (Just make sure to close them.)
-   Included a .NET version of asar.dll. Note that it still uses unmanaged types.
-   Asar now accepts org Label if the label is defined prior to use. (It's still not allowed to jump to a label that's defined later in the patch.)

### Bug fixes:

-   If a relative branch is out of bounds, Asar no longer throws "labels keep moving around" errors.
-   Removed the strange size hex value warnings.

- - -

## v?.??

**Release:** November 14, 2011

### Contributors:

-   Alcaro

### Bug fixes:

-   The autoclean warning for the end of a freespace area has been fixed. Again.
-   The freespace finder no longer skips a RATS tag if it's preceeded by an unprotected 00.

- - -

## v?.??

**Release:** ?

### Contributors:

-   Alcaro

### Bug fixes:

-   autoclean no longer complains if it attempts to overwrite something that isn't a valid SNES pointer.

- - -

## v?.??

**Release:** November 13, 2011

### Contributors:

-   Alcaro

### New features:

-   if statements have been made much more powerful. The old action if the statement is false was skipping the rest of the line if the statement is false; the new action is skipping the rest of the line, or if the if statement is at the end of the line, it skips everything until an endif. Nested if statements are allowed, as are else statements. (Note that you can't use else on one-line if statements.)
-   New command: prot. If used directly after a freecode or freedata command (that is, at the start of the RATS tag), it may contain a list of labels (up to 80 or something). The freespace blocks pointed to by these labels are removed if the freespace area is cleared by an autoclean. Example: org $008000 : autoclean JSL Main : freecode : prot Mydata : Main: LDA Mydata : RTL : freedata : Mydata: db $12,$34,$56,$78
-   Labels may be defined as Main(), not just Main:. This has been noted in the documentation.
-   Asar can now be built as a DLL file, but it's not tested very throughly.
-   'x' = $1234 is now a valid operation. It's equivalent to table file, but it accepts math and it's one file less to keep track of.
-   db 'x'+4 is now a valid operation.
-   freecode and freedata can now expand the ROM if needed.
-   !a = abc : !{a}def is now valid operation; it's equivalent to abcdef. You can nest defines inside the !{}s; a!{b!{c!def}g}h is accepted (assuming all defines exist).

### Bug fixes:

-   autoclean now works if called from $008000.
-   autoclean now works if aiming for the last eight bytes of a RATS tag.
-   autoclean now refuses to protect anything that isn't a label, for example math or constants, unless using the two-parameter method.
-   autoclean now refuses to protect a label at the end of a freespace block, since it'll think the RATS tag after that is the one that should be removed.
-   db 1/Label : Label: is now accepted, instead of being treated as a division by zero.
-   ($12),x is an error in xkas, since that addressing mode doesn't exist. However, Asar accepts parentheses and treats it as LDA $12,x, which isn't what the user meant. Therefore, it now emits a warning if this happens.

### Deprecated features:

-   !a equ $1234 is no longer valid.

- - -

## v?.??

**Release:** November 7, 2011

### Contributors:

-   Alcaro

### New features:

-   set fastrom is now documented in stdlib.asm.
-   Minor edits to the documentation.
-   An xkas emulation mode was added, which makes Asar act more like xkas 0.06. It should hopefully be 100% compatible with all previously submitted patches/sprites/etc.
-   Asar can now fix the checksum.
-   The command line options have been edited a bit, including a few bugfixes.

### Bug fixes:

-   Opcodes that may or may not take an argument (INC, LSR, etc) now work better together with autocolon.
-   "Unknown command" errors have been edited.

### Deprecated features:

-   set resizable has been removed.

- - -

## v?.??

**Release:** October 25, 2011

### Contributors:

-   Alcaro

### New features:

-   labelopt is no longer off
-   A lone { or } is now treated as a null command (for use with code folding). Note that they're not allowed elsewhere. They don't need to be matched.

### Bug fixes:

-   autoclean no longer kills inappropriate data if pointed to an unused area. Operator precendence screwed up stuff.

- - -

## v?.??

**Release:** ?

### Contributors:

-   Alcaro

### Bug fixes:

-   Asar will no longer place rats tags at any part of the unexpanded area, not even if it's a long list of 00s
-   Fixed a bunch of odd bugs with the automatic size finder and label optimizer, including one that treated LDA.l Label,x as LDA.w Label,x

- - -

## v?.??

**Release:** October 23, 2011

### Contributors:

-   Alcaro

### New features:

-   Blank lines are now allowed in macros.

### Bug fixes:

-   Broken or unknown macro arguments no longer abort assembling the rest of the macro.

### Deprecated features:

-   Removed the command DIEDIEDIE, a useless command that intentionally froze Asar if it's called from $088000 or later.

- - -

## v?.??

**Release:** October 23, 2011

### Contributors:

-   Alcaro

### Bug fixes:

-   Fixed relative forward branches in freespace too.

- - -

## v?.??

**Release:** October 23, 2011

### Contributors:

-   Alcaro

### New features:

-   How quickly Asar aborts if it finds errors can be set
-   autocolon now accepts db $20, $30
-   autoclean JSL.l is now valid

### Bug fixes:

-   Fixed some uninitialized value errors that created garbage error messages if the patch doesn't exist
-   Putting a label in front of an opcode messed up earlier; this has been fixed
-   freecode forgot setting a few variables, which creates a bunch of errors if it's used

- - -

## v?.??

**Release:** October 18, 2011

### Contributors:

-   Alcaro

### Bug fixes:

-   Added a missing else statement on the label optimizer disabling flag that made it turn it on, or get values that change in crappy ways, for everything.

- - -

## v?.??

**Release:** October 18, 2011

### Contributors:

-   Alcaro

### New features:

-   Asar can now assemble SPC700 code, though very little testing was done on this and a lot of bugs may still be included. Also note that mov a,(x)+ has been moved to mov a,(x+), since that opcode increases X and not the value X points to. Two lines in arch-spc700.cpp can be uncommented to reenable the first syntax.
-   The fastrom command has been moved to set fastrom on.
-   Readded the base command.
-   Other small changes.

### Bug fixes:

-   The first line of stdlib is no longer ignored (a variable wasn't initialized).
-   Assembling is aborted if errors are detected, instead of continuing. As a side effect from this, errors are no longer printed twice or thrice.

### Deprecated features:

-   rep Label has been blocked. It was never supposed to be allowed.

- - -

## v"0.30 or 0.40 or something"

**Release:** October 11, 2011

### Contributors:

-   Alcaro

### New features:

-   LDA $9E,y is now treated as LDA $009E,y, but it prints a warning.
-   New command: set. It can set various options, including expected ROM title (the romtitle command has been removed), if the .l->.w optimizer should be active, and if warnings should be shown.
-   Asar can now automatically (try to) add colons where it thinks they should be. Note that this is disabled by default and not recommended for anything serious.
-   As with the colon adder, it should be avoided unless there is a good reason to use it (for example running it from an IRC bot). (Note that a time limit should be enabled when running it from an IRC bot so that noone can freeze the bot with slow patches).

### Bug fixes:

-   Unknown command errors have been made saner. The old method was some sort of debug code.
-   Carriage returns are now ignored on Linux.
-   Assembling blank patches on nonexistent ROMs or ROMs with length zero has been fixed.

- - -

## v?.??

**Release:** October 9, 2011

### Contributors:

-   Alcaro

### New features:

-   !a += +1 has been implemented.

### Bug fixes:

-   If you type "freespace", the tool gives a more detailed error message that "Unknown command."
-   A bug related to using pushpc inside a freespace has been fixed.

### Notes

-   The tool has been renamed to Asar. Pronounciation: The first A is short, and the second is long (as in "Bazaar", except without the B). It doesn't matter if the S is pronounced as S or Z.

- - -

## v?.??

**Release:** October 8, 2011

### Contributors:

-   Alcaro

### New features:

-   It now looks for and assembles stdlib.asm. However, it prints an error if it doesn't find it.
-   Expected ROM title can now be set, see stdlib.asm for details. It defaults to SUPER MARIOWORLD if omitted, and it may not be set outside of stdlib.asm.
-   A 64-bit version has been included.
-   Both autoclean and autoclear works now. They act identically.

- - -

## v?.?? (Initial Release)

**Release:** October 6, 2011

### Contributors:

-   Alcaro

### New features:

-   Parentheses can be used, which allows some previously impossible statements.
-   Defines can be made longer without risk for crashes. This makes it much easier to implement Hijack Everywhere.
-   An if statement has been included, to get rid of the need for including those rep -1 tricks.
-   Freespace can be set automatically (it even includes a simple way to reclaim freespace used by older versions of the patch)

### Notes

-   In this original release, the tool was named "a.as"
