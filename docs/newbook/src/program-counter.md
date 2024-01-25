# Program Counter

The program counter (short: pc) refers to the position in the ROM at which Asar currently writes assembled code and/or data. It advances automatically whenever Asar writes to the ROM and is affected by the current [mapping mode](./mapping-modes.md), as well as a number of special commands. Note that all commands affecting the pc that take an address expect an SNES address and thus are also affected by the current mapping mode.

## `org`

{{# syn: org {snes_address} #}}

The org command directly sets the pc to `snes_address`. Most commonly used inside patches to specify which code to hijack or which data to overwrite.

```asar
org $008000
MainEntryPoint:
    ; ...
```

## `base`

{{# syn: base {snes_address/off} #}}

The base command makes Asar act as though the pc was currently set to `snes_address` without actually setting it; `base off` deactivates this behavior. This can be useful for writing code that you plan to execute from another location (such as RAM).

```asar
org $008000
MainEntryPoint:
    ; Some code which copies SomeRamRoutine to $7E0000 goes here
    ; ...
    jsl $7E0000
    ; ...

SomeRamRoutine:
base $7E0000
    ; ...
base off
    rtl
```

## `skip`

{{# syn:
skip {num_bytes}
skip align {alignment} [offset {offset}]
#}}

The skip command moves the pc by `num_bytes` bytes. By specifying a negative value, the pc can be moved backwards. When `alignment` is given, skips to the next multiple of `alignment`, plus `offset` if it is specified. Note that the alignment must be a power of 2, if specified. Offset can also be negative, in that case it's treated exactly like `alignment+offset`. The seeked-to position will always be after the current SNES position, but it might be before the next multiple of `alignment`: see the last example.

```asar
org $008000
skip 5
; pc is now at $008005
skip -1
; pc is now at $008004
skip align 16
; pc is now at $008010
skip align 16 offset 5
; pc is now at $008015
skip align $20 offset $17
; pc is now at $008017
```

## `warnpc`

{{# syn: warnpc {snes_address} #}}

The warnpc command checks if the current pc is `> snes_address`. If that's the case, it throws an error. This is useful for detecting overflow errors.

```asar
org $008000
incbin datafile.bin
warnpc $008100      ; Throws an error if datafile.bin is larger than $100 bytes.
```

## `bank`

{{# syn: bank {data_bank/noassume/auto} #}}

The bank command makes Asar's label optimizer act as though the current data bank was set to `data_bank`. Consider the following example:

```asar
bank $FF
        
lda DataTable,x

DataTable:
    db $01,$02,$03,$04
```

Asar will always assemble the `lda DataTable,x` with 24-bit addressing, unless the current pc (or [base address](#base)) is inside bank `$FF` itself. This is intended for code that uses a data bank register different from the code bank register. You can use `bank noassume` to make Asar act as though the data bank was always in a different bank. Using `bank auto` restores the default behavior of assuming that the data bank register and the code bank register are the same. Note that the bank command can't point to freespace areas.

```asar
org $008000
phb
lda #$FF
pha
plb

bank $FF
; ...
bank auto

plb
```

## `dpbase`

{{# syn: dpbase {snes_address} #}}

The `dpbase` command makes Asar's label optimizer assume the Direct Page register is set to the specified address. When used with the `optimize dp` command, this will cause Asar to use 8-bit addressing where possible. For example, in the following code Asar can assemble `lda SpriteTable,x` as a direct page address.

```asar
SpriteTable = $7E0200
dpbase $0200
optimize dp ram

org $008000
lda SpriteTable,x
```

## `optimize dp`

{{# syn: optimize dp {none/ram/always} #}}

This command changes how aggressive Asar's direct page access optimizer is. With `optimize dp none` (the default), the direct page optimizer is disabled and direct page accesses will only be done with the `.b` instruction suffix or with explicit addresses like `lda $42`. With `optimize dp ram`, direct page optimization will be performed according to the [dpbase](#dpbase) setting, but only on labels in bank `$7E`. With `optimize dp always`, direct page optimization will be performed on all labels in banks that have RAM mirrors, i.e. 00-3F and 80-BF, and also on labels in bank 7E.

## `optimize address`

{{# syn: optimize address {default/ram/mirrors} #}}

This command changes how aggressive Asar's label optimizer is. With `optimize address default`, references to labels will be shortened to 2 bytes only if the label is in the current data bank. With `optimize address ram`, additionally labels between `$7E:0000-$7E:1FFF` will be shortened to 2 bytes if the current data bank has RAM mirrors (`$00-$3F` and `$80-$BF`). With `optimize address mirrors`, additionally labels between `$00-3F:2000-7FFF` (that is, `$00:2000-$00:7FFF` all the way up to `$3F:2000-$3F:7FFF`) will be shortened to 2 bytes whenever the current data bank has RAM mirrors. Note that in [freespace](#freespace), the current bank will be assumed from whether the freespace was started as `freecode` or `freedata`, not where the freespace was actually placed in the end.

## `pushpc` / `pullpc`

The {{# cmd: pushpc #}} command pushes the current pc to the stack, the {{# cmd: pullpc #}} command restores the pc by pulling its value from the stack. This can be useful for inserting code in another location and then continuing at the original location.

```asar
org $008000
        
Main:
    jsl CodeInAnotherBank

pushpc
org $018000

CodeInAnotherBank:
    ; ...
    rtl
    
pullpc

bra Main
```

## `pushbase` / `pullbase`

The {{# cmd: pushbase #}} command pushes the current base to the stack, the {{# cmd: pullbase #}} command restores the base by pulling its value from the stack.

```asar
base $7E2000
        
InsideRam:
    jsl OutsideOfRam
    ; ...

pushbase
pushpc
base off

freecode

OutsideOfRam:
    ; ...
    jsl InRamAgain
    rtl

pullpc
pullbase

InRamAgain:
    ; ...
    rtl

base off
```
