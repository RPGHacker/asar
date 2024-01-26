# Functions

Functions in Asar can be considered the math equivalent of macros. They are a convenient way of wrapping commonly used math statements, can include parameters and can be called in all places where math is supported. Use the following syntax to define a function:

{{# hiddencmd: function {identifier}(...) = {math} #}}
```asar
function {identifier}([param1_name[, param2_name...]]) = {math}
```

where all the names can contain any of the following characters: `a-z A-Z 0-9 _` and where `{math}` can be any [math statement](./math.md) supported by Asar (including the use of other functions). Use a parameter's name to expand it inside a function.

```asar
function kilobytes_to_bytes(kb) = kb*1024
function megabytes_to_kilobytes(mb) = mb*1024
function megabytes_to_bytes(mb) = kilobytes_to_bytes(megabytes_to_kilobytes(mb))

; Will print "4 MB = 4194304 bytes."
print "4 MB = ",dec(megabytes_to_bytes(x))," bytes."


function data_index_to_offset(index) = index*2

lda .Data+data_index_to_offset(2)    ; Will load $0002 into A

.Data
    dw $0000
    dw $0001
    dw $0002
```

Function definitions must be on a single line and can't include whitespace in their math statements, except when using the [multi-line operator `\`](./formatting.md#multi-line-operators), which can be used to split long function definitions into multiple lines.

Note that user-defined functions can't use string parameters themselves. However, they can take strings as arguments and pass them on to built-in functions.

```asar
function readfilenormalized(filename, pos) = readfile4(filename, pos)/2147483648.0
db readfilenormalizd("datafile.bin", 0)
```

## Built-in Functions

Aside from user-defined functions mentioned above, Asar also supports a number of built-in functions. Some built-in functions take string parameters, which must be wrapped in double quotes.

<!-- TODO: better way to format this list?? -->

- `read1(pos[, default])`, `read2(pos[, default])`, `read3(pos[, default])`, `read4(pos[, default])`
  
  Read one/two/three/four byte(s) from the output ROM at SNES location pos. Mainly intended for detecting the presence of certain hijacks/patches in a ROM. Throws an error when given an invalid address, unless the optional parameter default is provided in which case it is returned.
  
  ```asar
  if read1($00FFD5) == $23
      !is_sa1_rom = 1
  else
      !is_sa1_rom = 0
  endif
  ```
  
- `readfile1(filename, pos[, default])`, `readfile2(filename, pos[, default])`, `readfile3(filename, pos[, default])`, `readfile4(filename, pos[, default])`
  
  Read one/two/three/four byte(s) from file filename at position pos (see section [Includes](./includes.md) for details on Asar's handling of file names). Throws an error when the referenced file doesn't exist or the given position is out-of-bounds, unless the optional parameter default is provided in which case it is returned.
  
  ```asar
  !readresult = readfile4("datafile.bin", 0, $FFFFFFFF)
  if !readresult != $FFFFFFFF
      print "Read $",hex(!readresult)," from datafile.bin."
  endif
  ```
  
- `canread1(pos)`, `canread2(pos)`, `canread3(pos)`, `canread4(pos)`, `canread(pos, num)`
  
  Returns 1 if reading one/two/three/four/num bytes from the output ROM at SNES location pos would succeed and 0 otherwise.
  
  ```asar
  if canread1($00FFD5) == 1
      print "Detected ROM type: $",hex(read1($00FFD5))
  else
      error "Failed to detect ROM type!"
  endif
  ```
  
- `canreadfile1(filename, pos)`, `canreadfile2(filename, pos)`, `canreadfile3(filename, pos)`, `canreadfile4(filename, pos)`, `canreadfile(filename, pos, num)`
  
  Returns 1 if reading one/two/three/four/num bytes from file filename at position pos would succeed and 0 otherwise (see section [Includes](./includes.md) for details on Asar's handling of file names).
  
  ```asar
  if canreadfile4("datafile.bin", 512) == 1
      print "Read $",hex(readfile4("datafile.bin", 512))," from datafile.bin at position 512."
  else
      error "datafile.bin either doesn't exist or is too small."
  endif
  ```
  
- `filesize(filename)`
  
  Returns the size of file filename. Throws an error if the file doesn't exist.
  
  ```asar
  !fsize #= filesize("datafile.bin")
  !fpos = 0
  
  assert !fsize >= 0, "datafile.bin doesn't exist or can't be opened".
  while !fpos < !fsize
      ; Do something with datafile.bin here, like calling readfile1("datafile.bin", !fpos)
      ;...
      
      !fpos #= !fpos+1
  endwhile
  ```
  
- `getfilestatus(filename)`
  
  Checks the status of file filename. Returns 0 if the file exists and can be read from, returns 1 if the file doesn't exist and returns 2 if the file exists, but can't be read from for any other reason (like being read-protected, being locked etc.).
  
  ```asar
  assert getfilestatus("datafile.bin") != 1, "datafile.bin doesn't seem to exist"
  ```
  
- `sqrt(x)`
  
  Computes the square root of x.
  
- `sin(x)`, `cos(x)`, `tan(x)`, `asin(x)`, `acos(x)`, `atan(x)`, `arcsin(x)`, `arccos(x)`, `arctan(x)`
  
  Various trigonometric functions. Units are in radians.
  
- `log(x)`, `log2(x)`, `log10(x)`
  
  Logarithmic functions (base-e, base-2 and base-10 respectively).
  
- `snestopc(address)`, `pctosnes(address)`
  
  Functions for converting between SNES and PC addresses. Affected by the current [mapping mode](./mapping-modes.md).
  
  ```asar
  print "SNES address $018000 in the current mapping mode is equivalent to PC address 0x",dec(snestopc($018000))
  ```
  
- `min(a, b)`, `max(a, b)`
  
  Return the minimum/maximum of two numbers.
  
  ```asar
  !start_index #= max(!current_index-1, 0)
  ```
  
- `clamp(value, minimum, maximum)`
  
  Makes sure that value stays within the bounds set by minimum and maximum. Equal to `min(max(value, minimum), maximum)`.
  
  ```asar
  !used_amount #= clamp(!used_percentage, 0.0, 1.0)*!total_amount
  ```
  
- `safediv(a, b, exception)`
  
  Returns `a/b` unless b is 0 in which case exception is returned. Intended for avoiding division by zero errors in functions.
  
  ```asar
  !single_sprite_memory = safediv(!total_sprite_memory, !max_num_sprites, 0)
  ```
  
- `select(statement, true, false)`
  
  Returns false if statement is 0 and true otherwise. Can be considered an if/else conditional that is usable within functions.
  
  NOTE: Asar always evaluates all parameters of a function before calling it, so if, for example, you pass an expression that divides by zero to select() as true, Asar will throw a division by zero error even if statement evalutes to 0 and thus false would be returned. To work around this, you can use the `safediv()` function in place of a regular division.
  
  ```asar
  function sprite_size() = select(!extra_bytes_enabled, 16+4, 16)
  ```
  
- `not(value)`
  
  Returns 1 if value is 0 and 0 in any other case. Useful for negating statements in the `select()` function.
  
  ```asar
  function required_sprite_memory(num_sprites) = not(!sprites_disabled)*sprite_size()*num_sprites
  ```
  
- `bank(value)`
  
  Returns `value>>16`
  
  ```asar
  lda #bank(some_label)
  ```
  
- `equal(value, comparand)`, `notequal(value, comparand)`, `less(value, comparand)`, `lessequal(value, comparand)`, `greater(value, comparand)`, `greaterequal(value, comparand)`
  
  Comparison functions. Return 1 if the respective comparison is true and 0 otherwise. Useful as statements in the `select()` function.
  
  ```asar
  function abs(num) = select(less(num, 0), num*-1, num)
  ```
  
- `and(a, b)`, `or(a, b)`, `nand(a, b)`, `nor(a, b)`, `xor(a, b)`
  
  Perform the respective logical operation with a and b. Useful for chaining statements in the `select()` function.
  
  ```asar
  function total_sprite_extra_bytes(num_sprites) = select(and(not(!sprites_disabled), !extra_bytes_enabled), 4, 0)*num_sprites
  ```
  
- `round(number, precision)`
  
  Rounds `number` to `precision` decimal places. Pass 0 as precision to round to the nearest integer.
  
  ```asar
  if round(!distance, 2) == 0.0
      error "Distance is zero or almost zero. Please choose a bigger value for distance as small values will cause problems."
  endif
  ```
  
- `floor(number)`, `ceil(number)`
  
  Rounds a number up (in the case of `ceil`) or down (in the case of `floor`) to the nearest integer.
  
  ```asar
  !banks_used #= ceil(!data_size/65536)
  ```
  
- `defined(identifier)`
  
  Takes an identifier as a string parameter and returns 1 if a define with that identifier exists, 0 otherwise.  
  NOTE: Don't include the `!` in the identifier as Asar will otherwise try to expand it as a define before calling the function.
  
  ```asar
  if defined("include_guard") == 0
      !include_guard = 1
      ; ...
  endif
  ```
  
- `sizeof(identifier)`
  
  Takes the identifier of a struct as a parameter and returns the base size of that struct (without any extension structs).
  
  ```asar
  struct parent $0000
      .data1: skip 2
  endstruct
  
  struct child extends parent
      .data2: skip 3
  endstruct
  
  db sizeof(parent)             ; db 2
  db sizeof(parent.child)       ; db 3
  ```
  
- `objectsize(identifier)`
  
  Takes the identifier of a struct as a parameter and returns the object size of that struct. In the case of an extended struct, this will be the base size of the struct plus the size of its largest extension struct. Throws an error if a struct with that name doesn't exist.
  
  ```asar
  struct parent $0000
      .data1: skip 2
  endstruct
  
  struct child extends parent
      .data2: skip 3
  endstruct
  
  db objectsize(parent)         ; db 5
  db objectsize(parent.child)   ; db 3
  ```
  
- `datasize(label)`
  
  Takes a given label and calculates the distance between it and the next label. It will throw a warning if the distance exceeds 0xFFFF or is the last label in the targeted assembly.
  
  ```asar
  org $008000
  main:
  
  lda #datasize(my_table)       ;3
  lda #datasize(other_label)    ;0x7FF3 (last label, throws a warning. calculated as $FFFFFF-$00800C)
  lda #datasize(main)           ;9
  
  
  my_table:
      db $00, $00, $02
  other_label:
  ```
  
- `stringsequal(string1, string2)`
  
  Returns 1 if the given string parameters are equal and 0 otherwise.
  
  ```asar
  if not(stringsequal("!assembler", "asar"))
      warn "This patch was only tested in Asar and might not work correctly in your assembler."
  endif
  ```
  
- `stringsequalnocase(string1, string2)`
  
  Returns 1 if the given string parameters are equal and 0 otherwise. The comparison is case-insensitive.
  
  ```asar
  if not(stringsequalnocase("!assembler", "ASAR"))
      warn "This patch was only tested in Asar and might not work correctly in your assembler."
  endif
  ```
- `pc()`

  Returns the current SNES address. This is a shorthand for placing a label right before the current command.
  
- `realbase()`
  
  Returns the current address in the ROM being written to. This is not the same as the value of a nearby label when the `base` command is active: it returns the actual address the code will end up at.
    
