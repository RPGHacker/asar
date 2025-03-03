# Standard Defines

Aside from passing additional defines to Asar via the command line, it's possible to also do so via a file called `stddefines.txt`. When a file with this name exists next to the Asar executable, Asar automatically opens it and adds every line in it as an additional define. The syntax is similar to Asar's regular define syntax, with a few notable differences. There are no spaces required around the `=`, the `!` of the identifier is optional, whitespace around the identifier is ignored, so is whitespace around the value (unless the value is delimited by double quotes, in which case any whitespace inside is kept in the define), the value itself is optional (when left out, it is set to an emptry string). Lines containing only whitespace are ignored. The purpose of this file is to make it easier to distribute standard code libraries for use with Asar by making it possible to just unpack the contents of a ZIP file or similar directly into the Asar directory. See section [Defines](./defines.md) for details on defines.

All of the examples below are valid:

```
!stddefined1=1
 stddefined2=1

stddefined3
stddefined4 = 1 
stddefined5 = " $60,$50,$40 "
```
