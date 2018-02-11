                               (Known) changes between Asar and xkas

This document assumes the reader is familiar with xkas. If this is not the case, please read xkas.html first.

New features:
- The base command now accepts base off, which makes it act like the code is at the current location
  again.
- incbin can now include parts of a file. Syntax: incbin "File name.bin":start-end, where start and
  end are hexadecimal numbers without $. Note that math is not allowed. An ending position of 0 will
  include the rest of the file.
- You can now put "-> Labelname" after an incbin, which is equivalent to pushpc : freedata align :
  Labelname: incbin file.bin : pullpc with special permission to cross bank borders. (If the file or
  file part is 32767 bytes or smaller, alignment isn't enforced. Size limit is 65536 bytes due to
  limits in the RATS tag format.)
- You can also use "incbin file.bin -> $123456" to if you want to include something larger than
  65536 bytes.

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