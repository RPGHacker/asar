                               (Known) changes between Asar and xkas

This document assumes the reader is familiar with xkas. If this is not the case, please read xkas.html first.

(Potentially) xkas incompatible bugfixes and changes:
If any of these dissatisfies you, put ;@xkas at the top of your patch and Asar will enter maximum
 xkas compatibility mode and fix all of these I have observed in practice. Most Asar-only features
 will still work, but they'll throw warnings everywhere.
- Various operations that give undesirable behaviour in all circumstances (for example inaccessible
  labels) will print errors in Asar. (A bunch of crashes have been removed as well, but that will
  obviously not break old xkas codes.)
- Asar prefers uppercase hex over lowercase in print pc. This may confuse crappy tools, but I don't
  think any tools we use around here act like that.

New commands:
- struct/endstruct: used to define a struct, which are basically just a convenient and easier-to-read way
  of accessing tables. The code for this was written by p4plus2 and I really can't explain it well, so
  just take a look at src/tests/structs.asm for usage examples.

Known bugs:
- JML.w and JSL.w aren't rejected; they're treated as JML.l and JSL.l. This is since Asar has a much
  more powerful size finding system than xkas, and automatically picks .w if it contains a label in
  the same bank - even for .l-only instructions like JML. Therefore, I'm forced to either reject JML
  and JSL to a label in the same bank, or allow JML.w, and I prefer allowing unusual but bad stuff
  over rejecting common and valid stuff. Workaround: Don't do this.
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