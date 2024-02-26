Asar Readme
===========

If you just want to apply patches, double click asar.exe and Asar will ask you
for the patch file and the ROM. You can also pass arguments on the command
line, they are documented in the manual (see below).

Writing Patches
---------------

For a detailed guide on all commands supported by Asar, see the included manual
(also available on-line at https://rpghacker.github.io/asar/asar_19/manual/ ).

Another thing that may be helpful when writing patches is syntax highlighting.
Currently asar includes syntax definitions for Notepad++ and Sublime Text.

To install it for Notepad++, go to Language -> Define your language..., then
click Import near the top and navigate to Asar's directory -> ext ->
notepad-plus-plus, and open syntax-highlighting.xml.
For Sublime Text, go to Preferences -> Browse Packages... -> User, and copy
ext/sublime-text/65c816.sublime-syntax there.

History
-------

For a detailed changelog, please see the included changelog (or on-line at
https://rpghacker.github.io/asar/asar_19/changelog/ ).

Bug reports
-----------

First, make sure the bug is in Asar, not the patch you are applying. When you
are sure that the problem is in Asar:

Asar's issue tracking is at https://github.com/RPGHacker/asar/issues . If you
find a bug, please first check that the issue hasn't been reported yet, and then
create a new issue, detailing what happens, what you expected to happen, and the
patch you are trying to apply.

If you don't have a GitHub account, you can also tell someone about the bug on
Discord (in the #asm channel on SMWC, or #asar on SnesLab) or the SMW Central
forums (in Asar's thread, https://smwc.me/t/51349 ).
