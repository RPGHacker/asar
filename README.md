                               (Known) changes between Asar and xkas

This document assumes the reader is familiar with xkas. If this is not the case, please read xkas.html first.

New features:
- If you're in a namespace and a label isn't found, Asar looks in the global namespace.
- This technically isn't a change, but Label = Address is now a well defined operation. While it
  exists in xkas6, the only ways to find it are by inspecting the output of export.label, by
  inspecting the source code, and by mistake (I've seen it done thrice, leading to odd bugs in
  most cases). The differences to normal labels are that it does not clear sublabels, they're
  always in the global namespace, and they don't use the current code pointer.
- Sublabels can now be multiple levels deep; ..Loop is a valid label name.
- + and - labels can be infinite depth. This means that "+++++" is a valid label name.
- + and - labels no longer piggyback on sublabels; you can go to them across other labels (and you
  can call a label .__br_pos1_0 if you want).
- The base command now accepts base off, which makes it act like the code is at the current location
  again.
- Parentheses are allowed at all places where math is expected.
- incbin can now include parts of a file. Syntax: incbin "File name.bin":start-end, where start and
  end are hexadecimal numbers without $. Note that math is not allowed. An ending position of 0 will
  include the rest of the file.
- You can now put "-> Labelname" after an incbin, which is equivalent to pushpc : freedata align :
  Labelname: incbin file.bin : pullpc with special permission to cross bank borders. (If the file or
  file part is 32767 bytes or smaller, alignment isn't enforced. Size limit is 65536 bytes due to
  limits in the RATS tag format.)
- You can also use "incbin file.bin -> $123456" to if you want to include something larger than
  65536 bytes.
- print can now accept freespaceuse, which counts all data handed out by freespace, freecode,
  freedata, and external incbin. That's everything print bytes is used for anyways, and pushpc makes
  print bytes completely confused, so here's an accurate counter.
- print can now accept dec(math) and hex(math), which prints the contents in base 10 or 16 (no zero
  padding). The contents may be any piece of math, including labels. Additionally, print accepts
  double(num) and double(num, precision) in case you want to print a double variable with its
  fractional part. You can specify the number of decimal places to print (0 to 100) as the second
  argument. If you only pass a single argument, a default of 5 decimal places are printed.
  Note that, like everything else, double() is affected by your "math round" setting, so unless
  you set math round to off, double() will behave just like dec().

(Potentially) xkas incompatible bugfixes and changes:
If any of these dissatisfies you, put ;@xkas at the top of your patch and Asar will enter maximum
 xkas compatibility mode and fix all of these I have observed in practice. Most Asar-only features
 will still work, but they'll throw warnings everywhere.
- Various operations that give undesirable behaviour in all circumstances (for example inaccessible
  labels) will print errors in Asar. (A bunch of crashes have been removed as well, but that will
  obviously not break old xkas codes.)
- Asar prefers uppercase hex over lowercase in print pc. This may confuse crappy tools, but I don't
  think any tools we use around here act like that.
- Asar initializes tables to garbage if you use table, while xkas initializes it to all 00.
- Asar prints errors and warnings to stderr instead of stdout. However, to keep compatibility with
  old tools, Asar will send the errors to stdout if it's renamed to xkas.exe.

New commands:
- freespace/freecode/freedata: Automatic freespace finders, including automatic RATS tags. freespace
  is expected to be followed by a comma separated list of details on which kind of freespace is
  required. Valid details are ram, noram, align, cleaned, static, and a $xx byte:
  - ram tells that the RAM mirrors must exist (which means the data must be in banks 10-3F), while
    noram tells that the RAM mirrors aren't needed (which makes Asar favor banks 40-6F and F0-FF,
    but it'll put it the patch the earlier banks if no freespace in the favored banks is available);
    it's invalid to not specify whether the RAM mirrors are needed.
  - align requires that the freespace begins at the start of a bank.
  - cleaned will suppress the warning about the freespace leaking; use this if you're cleaning it
    with an autoclean on a couple of read1() instructions Asar can't figure out.
  - static tells that the freespace may not move, once assigned; any attempt to make it grow will
    result in an error being thrown.
  - $xx byte argument (e.g. $FF) tells freespace to search for contiguous chunks of that byte.
    Default is $00 if not supplied. Autoclean will clean the ROM using this byte also.
  freecode is a shortcut to freespace ram, and freedata is a shortcut to freespace noram.
- autoclean: This one will remove a freespace block in an existing ROM. It must not be placed in
  freespace, and it must be followed by JML, JSL, dl, or a mathematical expression (for example
  "autoclean JSL Mymain"). This is so the same patch can be applied to the same ROM twice without
  freespace leaks. If there's no JSL/etc there (for example A9 00 85 19), or if it's not in the
  expanded area (for example 22 06 F6 00), it will not clear anything (in case the patch wasn't
  applied earlier); for this reason, it's recommended to put a JSL or JML there. The command after
  it will be assembled. If you want to remove some data only pointed to indirectly, you may use
  autoclean $108000 (math allowed), where the argument evaluates to any place in the RATS tag or the
  RATS protected data. It is safe to place autoclean on multiple JSLs or JMLs to the same freespace.
  Note that you may not aim for a label at the end of a freespace block; that will just confuse
  Asar.
- prot: If used directly after a freecode or freedata command (that is, at the start of the RATS
  tag), it may contain a list of comma separated labels (the limit is around 80). If the freespace
  area with the prot tag is removed by an autoclean, the freespace area pointed to by the prot will
  also be reclaimed. This is intended for large data blocks that are only pointed to from inside a
  freecode statement and therefore can't be autocleaned directly. Note that your main code will need
  an autoclean, or everything will be leaked; Asar can't solve circular dependencies, and won't even
  try.
- pushpc/pullpc: In case you want to put code at one place instead of two.
- function: Defines a function. For details, see below.
- if/elseif/else/endif: For conditional compilation. See section "Conditionals" below for details.
- while: For repeated compilation of the same code. See section "Loops" below for details.
- assert: Accepts an if-like condition; if the condition is false, it prints an error message.
- math: Changes how the math parser works. It takes two arguments: First, it wants a setting name
  (valid settings are "pri" and "round"), then it wants "on" or "off" depending on whether you want
  it on or off. pri tells it to apply prioritization rules (exponentiation comes before
  multiplication/division, which comes before addition/subtraction, and so on). round tells whether
  Asar should round the intermidate values down to the closest integer after each operation, like
  xkas does. Because some Asar users are used to the (rather strange) behaviour of xkas,
  prioritizing is left-to-right by default, and rounding is on. Parentheses are allowed no matter
  what these settings are.
- warn: Controls a few warning-related settings. Its syntax is similar to math, but the only valid
  flag is "xkas" (warns about a few things that Asar does differently from xkas). Note that you
  should turn off xkas emulation mode with this on; some changes throw warnings and errors in native
  mode even with this one off, so I didn't make this one do anything to those.
- bank: Makes the label optimizer act like you're in another bank. This is not the same as base;
  bank $FF : LDA Label,x : Label: db $01,$02,$03,$04 will use 24-bit addressing for the LDA
  (assuming the current base address isn't in bank $FF). To make it assume you're never in the same
  bank, use bank noassume. bank auto will make it act like it's back in the current (base) bank. The
  purpose of this command is long codes that assume the data bank register is not the same as the
  code bank register. Note that you can't point it to freespaced areas. (Yes, this is the same as
  xkas' assume db, but that command has a very weird syntax, and since no other part of assume is
  planned for Asar, I prefer syntax that makes sense over backwards compatibility with a grand total
  of zero patches.)
- @include, @includefrom: Tells that an asm file may not be assembled directly, but must be included
  from another file. @includefrom tells which file it's supposed to be included from, though it
  doesn't verify that it really is included from this file. It's just a sanity check.
- struct/endstruct: used to define a struct, which are basically just a convenient and easier-to-read way
  of accessing tables. The code for this was written by p4plus2 and I really can't explain it well, so
  just take a look at src/tests/structs.asm for usage examples.

Conditionals:
Conditionals allow a piece of code to only be assembled under certain circumstances. The syntax of a
  condition is either "42" (evaluates to true if the value is 1 or higher), "42 > 24" (the middle
  argument can be any of <, >, <=, >=, != and ==), or any number of the before with " && " or
  " || " in between (you can only use one of those two in the same condition). Evaluation is lazy;
  "0 && ###" and "1 || ###" will not throw errors.
The commands that accept conditionals are if, elseif, else, and endif (and assert, but that one
  doesn't skip any code). The behaviour should be obvious.
You generally want a couple of readN or user-changeable defines in a condition, but if 0 is a way to
  comment out large blocks of code at once.
  
Loops:
The syntax and functionality are very similar to if conditionals, except you use "while" instead
  of "if" and the condition is evaluated repeatedly until it becomes < 1 instead of just once.
  The code block inside the while loop is assembled as many times as the condition remains >= 1.
  This can be used to generate big data tables without having to rely on macro recursion.
  Internally, while loops are implemented as a variation of if conditionals for reasons of
  simplicity and compatibility, so they always need to end on an endif. Note that else and elseif
  statements are not allowed in combination with a while loop, though. When using while loops, be
  careful not to cause an infinite loop, as Asar won't make any effort to detect those.

Functions:
Functions are what they sound like: They take zero or more arguments, and return another value. They
  can be used anywhere where other math is allowed, and they're defined with the keyword function.
Example:
function f(x) = x*x
db f(9);same as db 81 (db $51 in hex)
The = is needed. I find it to look much nicer than without it.
There are also a few predefined functions:
read1, read2, read3, read4 - Reads a few bytes from the ROM at the indicated SNES location. They
  read one, two, three, or four bytes, respectively. They're mainly intended to allow hijacking a
  patch that moves around (read3 is best suited for this), or verifying that a needed patch is
  installed (assert and read1 is probably the best combination for this, though if may also be
  useful). If you give it a second argument, it'll return that instead of throwing an error if the
  address is invalid.
readfile1, readfile2, readfile3, readfile4 - Same as above, except bytes are read from another file.
  Pass the filename of the file to read as the first argument (wrapped in double quotes) and the
  offset into that file as the second argument. Can also be passed a third argument which is returned
  as a default value if reading fails.
  Usage example:
    readfile1("mydatafile.bin", 512)
canread1 through 4, canread - Returns 1 if read1 through 4 from the specified address will succeed;
  canread takes a second argument telling how many bytes you need to read.
canreadfile1 through 4, canreadfile - Same as above, but for external files. Pass the filename
  as the first argument (wrapped in double quotes), the offset into the file as the second
  argument and the number of bytes to read as the third argument (in the case of canreadfile).
sqrt, sin, cos, tan, asin, acos, atan, arcsin, arccos, arctan, log, log10, log2 - Some random
  functions. I'm pretty sure they're totally useless in an assembler, but I felt that just four
  functions is a bit too little. (They're older than canread#.)
snestopc, pctosnes - Manually convert an address from PC to SNES format or vice versa.
  Both functions are dependent on the ROM's current mapping mode, so use them with caution.
max, min, clamp - max() and min() return the maximum or minimum of two values, whereas
  clamp is passed three arguments and assures that the first argument is >= the second argument
  and <= the third argument.
  Examples:
    clamp(5, 0, 10)   =>  5
	clamp(15, 0, 10)  =>  10
	clamp(-5, 0, 10)  =>  0
safediv - Performs division with first and second argument, except if second argument is zero, in
  which case it returns the third argument. Useful to prevent "division by zero" errors.
  Example:
    safediv(10, 0, 1)  =>  1
select - Functions very similar to if/else conditionals, except it can be used insied functions
  and calculations. The first argument passed to it as treated as a statement. If this argument
  resolves to anything non-zero, the second argument passed to select is returned. If the statement
  resolves to zero, the third argument passed to select is returned. Please note that unlike
  blocks in if/else conditionals, function arguments in Asar are always evaluated before a function
  is called. In other words: if you do select(1, 1/1, 1/0), Asar will throw a "division by zero"
  error, even though the function would return 1/1 and disregard the 1/0. In this particular case,
  it's recommended to simply use the safediv() function in place of a regular division.
not - Returns 1 if passed a 0 and 0 in any other case.
equal, notequal, less, lessequal, greater, greaterequal - Rather self-explanatory, return 1 if the
  respective comparison is true and 0 otherwise. Primarily intended for being passed as statement
  to the select() function.
and, or, nand, nor, xor - Also self-explanatory, return 1 if the respective logical operation is
  true and 0 otherwise. Primarily intended for being passed as statement to the select() function.
round - Rounds the first argument to the number of decimal places specified by the second argument.
  Pass 0 as the second argument to round to the nearest integer.
All built-in functions can be overridden by user-defined functions if you want. Prepending an
  underscore (for example _read3()) leads to the same function; this is so you can override read3
  with a function that calls _read3.

Known bugs:
- JML.w and JSL.w aren't rejected; they're treated as JML.l and JSL.l. This is since Asar has a much
  more powerful size finding system than xkas, and automatically picks .w if it contains a label in
  the same bank - even for .l-only instructions like JML. Therefore, I'm forced to either reject JML
  and JSL to a label in the same bank, or allow JML.w, and I prefer allowing unusual but bad stuff
  over rejecting common and valid stuff. Workaround: Don't do this.
- If two freespaced codes are put in the same bank, Asar will use 24bit addressing for access
  between them, even though 16bit is possible. Workaround: Merge the freespaces (use pushpc and
  pullpc if needed), or ignore it. Do not demand explicit .w addressing, since you have no guarantee
  that they'll be put in the same bank for everyone, even if they do for you.
- You can't have more than 125 freespaced areas in the same patch, due to internal limitations in
  how labels work. Workaround: Guess three times.
- If Asar wants to put two freespaced areas beside each other, a few bytes will be wasted between
  them. This is due to how the size definer works (it assumes .l addressing for everything on the
  first pass), and I can't fix it without using four passes, and I don't believe that's worth it.
  Workaround: You don't really need to care about it, it's less than 1% of the code size in all
  serious situations, and if you only use one freespace, there is no spill. If you're sure you want
  to care, merge the freespaces or make them so big they won't fit in the same bank.
- Due to how my define parser works, Label: !Define = Value will create error messages. Workaround:
  Label: : !Define = Value. However, Label: Anotherlabel = Value will work.
- rep 2 : rep 3 : NOP won't give six NOPs, it will give three. Workaround: Multiply them with each
  other, if you need to do that at all. (This bug also exists in xkas.)
- else is treated as elseif 1 in all contexts, so attaching two else commmands to the same if
  clause, or putting an elseif after an else, is not rejected (though it is rather dumb to do that).

Deprecated features:
All of these should be avoided; they're only listed here to make sure people don't claim they've
  found any easter eggs. They may start throwing warnings in newer versions of Asar.
- if a = b
- fastrom