# Binary Data

Asar supports a number of commands which allow you to insert binary data directly into the ROM.

## Tables

{{# syn:
db {value}[,value...]
dw {value}[,value...]
dl {value}[,value...]
dd {value}[,value...]
#}}

Table commands let you insert a number or a list of numbers directly into the ROM as raw bytes. Use `db` for 8-bit numbers, `dw` for 16-bit numbers, `dl` for 24-bit numbers and `dd` for 32-bit numbers respectively, where `value` can be a number literal, a math statement, a label or a Unicode string delimited by double quotes. When using `dw`, `dl` or `dd`, each number is converted to little-endian. Big numbers are truncated to smaller integers as needed.

```asar
org $0189AB
Label:

; This will write the following data to the ROM:
; $01  $03  $07  $AB  $41 $42 $43
db $01,$0203,$04050607,Label,"ABC"
; This will write the following data to the ROM:
; $01 $00  $03 $02  $07 $06  $AB $89  $41 $00 $42 $00 $43 $00
dw $01,$0203,$04050607,Label,"ABC"
; $01 $00 $00  $03 $02 $00  $07 $06 $05  $AB $89 $01  $41 $00 $00 $42 $00 $00 $43 $00 $00
dl $01,$0203,$04050607,Label,"ABC"
; $01 $00 $00 $00  $03 $02 $00 $00  $07 $06 $05 $04  $AB $89 $01 $00  $41 $00 $00 $00 $42 $00 $00 $00 $43 $00 $00 $00
dd $01,$0203,$04050607,Label,"ABC"
```

By default, each character in a Unicode string used in a table maps onto the respective Unicode code points it's composed of. This mapping can be customized via character literal assignments:

{{# syn: '{character}' = {value} #}}

Where `character` is a Unicode code point and `value` is any math expression, specifying what value that code point will be remapped to. Only single code points can be remapped at this time - e.g., a precomposed "Ä" will work, while a split "¨" and an "A" will throw an error.  
To reset all mappings to a direct Unicode code point mapping, use the command {{# cmd: cleartable #}}. Additionally, the {{# cmd: pushtable #}} command lets you push all current mappings onto a stack, whereas the {{# cmd: pulltable #}} command lets you restore the mappings from that stack.

```asar
; Contents of table1.asm:
;'A' = $1A
;'B' = $1B
;'C' = $1C
;'日' = $2A
;'本' = $2B
;'語' = $2C

; Contents of table2.asm:
;'A' = $1D
;'B' = $1E
;'C' = $1F
;'日' = $2D
;'本' = $2E
;'語' = $2F

; This writes $41 $42 $43 $E5 (from U+65E5) $2C (from U+672C) $9E (from U+8A9E)
db "ABC日本語"

incsrc "table1.asm"

; This writes $1A $1B $1C $2A $2B $2C
db "ABC日本語"

pushtable
incsrc "table2.asm"

; This writes $1D $1E $1F $2D $2E $2F
db "ABC日本語"

pulltable

; This writes $1A $1B $1C $2A $2B $2C
db "ABC日本語"

cleartable

; This writes $41 $42 $43 $E5 $2C $9E
db "ABC日本語"

'A' = $20
'B' = $20+1
'C' = $20+2
'日' = $20+3
'本' = $20+4
'語' = $20+5

; Those both write $20 $21 $22 $23 $24 $25
db "ABC日本語"
db 'A','B','C','日','本','語'
```

Note that Asar tries to replace defines wherever possible - even inside strings. Sometimes, this might be undesired. In those cases, you can prefix the `!` with a `\` to escape it. The `\` itself can be escaped with another `\`. In the case of a `"` it can be escaped with an additional `"`.

```asar
!define = "text"

; This writes "text" to the ROM
db "!define"

; This writes "!define" to the ROM
db "\!define"

; This writes "\text" to the ROM
db "\\!define"
; This writes 'something "cool"' to the ROM
db "something ""cool"""
```

## `fillbyte` / `fill`

{{# syn: fillbyte {byte} #}}

{{# syn:
fill {num}
fill align {alignment} [offset {offset}]
#}}

The fillbyte and fill commands let you write a specific byte value to the ROM multiple times. The `byte` parameter of fillbyte specifies which value to write, wheres fill writes that value to the output ROM `num` times. If `alignment` is specified, the value will be written repeatedly until the SNES address has the specified alignment, similar to [`skip align`](./program-counter.md#skip).

```asar
fillbyte $FF
; This writes $FF $FF $FF $FF $FF $FF $FF $FF
fill 8
org $008005
; this writes $FF until SNES address $00800A (=$8008 + 2)
fill align 8 offset 2
```

It's also possible to write 16-bit, 24-bit or 32-bit values with the fill command by using {{# cmd: fillword #}}, {{# cmd: filllong #}} or {{# cmd: filldword #}} instead of `fillbyte`. Note that the `num` parameter of fill still specifies the number of bytes to write in those cases. Values might get truncated as needed to exactly reach the specified number of bytes to write.

## `padbyte` / `pad`

{{# syn: padbyte {byte} #}}
{{# syn: pad {snes_address} #}}

The `padbyte` and `pad` commands let you write a specific byte value to the ROM until the pc reaches a certain SNES address. The `byte` parameter of padbyte specifies which value to write, wheres pad writes that value to the output ROM until the pc reaches `snes_address`.

```asar
org $008000
padbyte $FF
; This writes $FF $FF $FF $FF
pad $008004
```

It's also possible to write 16-bit, 24-bit or 32-bit values with the pad command by using {{# cmd: padword #}}, {{# cmd: padlong #}} or {{# cmd: paddword #}} instead of `padbyte`. Note that the `snes_address` parameter of pad still specifies the end offset of the write in those cases. Values might get truncated as needed to exactly reach the specified end offset.

## `incbin`

{{# syn: incbin {filename}[:range_start..range_end] #}}

The incbin command copies a binary file directly into the output ROM. The `filename` parameter specifies which file to copy (enclose in double quotes to use file names with spaces, see section [Includes](./includes.md) for details on Asar's handling of file names) and the optional `range_start` and `range_end` parameters are math expressions which specify a range of data to copy from the file (a `range_end` of 0 copies data until the end of the file; not specifying a range copies the entire file).

```asar
; datafile.bin contains the following bytes:
; $00 $01 $02 $03 $04 $05 $06 $07 $08 $09 $0A $0B $0C $0D $0E $0F

; This writes $00 $01 $02 $03 $04 $05 $06 $07 $08 $09 $0A $0B $0C $0D $0E $0F
incbin "datafile.bin"

; This writes $09 $0A $0B $0C $0D $0E
incbin "datafile.bin":$9..$F

; This writes $01 $02 $03 $04
incbin "datafile.bin":$F-$E..2+3
```
