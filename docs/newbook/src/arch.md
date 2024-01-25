# Architectures

Asar supports a number of different target architectures for code compilation. They can be activated via the command {{#cmd: arch {name} #}}. Going into detail on any of the supported architectures is beyond the scope of this manual. For that, it's recommended to check the SNES Dev Manual or other specialized resources. Asar tries as much as possible to always stick to the known conventions and specifications of each respective architecture (with a few notable exceptions that are hopefully all covered somewhere in this manual).

## Supported architectures

- `65816`: Compiles code for the 65C816, used by the main SNES CPU and by SA-1. This is the default architecture. It supports the syntax recommended by WDC, with the exception of the `MVN` and `MVP` instructions. See the instruction list page for details.
- `spc700`: Compiles code for the SPC700 CPU, the audio coprocessor in the SNES. Follows the format the SNES Dev Manual recommends, with the exception of `mov (x)+,a` and `mov a,(x)+`, which are moved to `mov (x+),a` and `mov a,(x+)`. See also the [`spcblock`](#spcblock) section for an alternative way of assembling SPC700 code.
- `superfx`: Compiles code for the SuperFX coprocessor.

All of Asar's features should be compatible with all of the supported target architectures, but it's not recommended to mix labels between different architectures as that will lead to undefined behavior. Opcodes in Asar are case-insensitive, which means that `LDA` and `lda` will be treated equally.

```asar
arch 65816
lda $00

arch spc700
mov a,$00
```

## Number Literals

<!-- TODO: move this to math.md instead?? -->

Asar supports decimal, hexadecimal and binary number literals. Hexadecimal literals use `$` as a prefix, binary literals use `%` as a prefix. Number literals can be made positive or negative by prefixing a `+` or a `-` (without a sign, positive is assumed). They can also be prefixed with a `~` to get their unary complement (a 32-bit integer with all the bits inverted).

```asar
lda $00
clc
adc #-10
and #%01111111
lda #~$80   ; Equal to lda #$FFFFFF7F
```

Aditionally, Asar supports character literals by delimiting a single Unicode character with `'`. Asar will automatically convert them to the integer value currently mapped to them (by default their Unicode code point). They can be used in all places where number literals can be used. See section [Tables](#tables) for details on character mapping.

```asar
lda #'a'
sta $00

db 'x','x'+1,'x'+2

db '💩'
```

## Opcode Length Specification

By appending `.b`, `.w` or `.l` to an opcode, you can specify that opcode's length. This is recommended in cases where the length could be ambiguous.

```asar
lda #0      ; Could be either lda #$00 or lda #$0000
lda.b #0    ; Always lda #$00
lda.w #0    ; Always lda #$0000
```

When no length is specified, Asar tries to guess the length based on the operand. Note that Asar does not use the standard `<>` for length specifications to avoid ambiguity with other uses of these symbols (such as in macros or math statements). Opcode length specifications are currently supported for the 65c816 and SPC700 architectures.

## Pseudo Opcodes

Pseudo opcodes are a convenience method of repeatedly using opcodes that don't take an operand. Instead of using the opcode multiple times, the following syntax can be used:

```asar
{opcode} #{num}
```

This assembles `opcode` `num` times in succession. This means that

```asar
nop #3

inx #2
```

is the same as

```asar
nop
nop
nop

inx
inx
```

## `spcblock`

SPC blocks are a convenient way of defining command data meant to be sent to the SPC700 in games using well-known SPC engines (though at this time, only the `N-SPC` engine is supported). The general format looks like this:

{{# hiddencmd: spcblock {target_address} [{engine_type}] #}}
{{# hiddencmd: endspcblock [execute {execution_address}] #}}
```asar
spcblock {target_address} [{engine_type}]
    [spc700_instructions...]
endspcblock [execute {execution_address}]
```

Inside an spcblock, `arch spc700` is automatically active (see [above](#supported-architectures) for details). The `target_address` parameter specifies the target address (in ARAM) for the command data. The optional `execute` parameter tells Asar to generate a "start execution" command immediately after this SPC block, with `execution_address` as the ARAM address to start execution at. The `engine_type` parameter specifies which SPC engine to use. When omitted, the default value of `nspc` is used. The following engine types are supported:

### `nspc`

This engine type implements the format used by the N-SPC engine found in most Nintendo games, as well as by the SPC700's initial program loader. The output format is:

```asar
dw <block_length>
dw <target_address>
<instructions...>
[dw $0000, <execution_address>]
```

Example usage:

```asar
                                   ; assembles to:
spcblock $6000 nspc                ; dw $0007 (length of the spcblock contents)
                                   ; dw $6000 (target address)
    db $00,$01,$02,$03             ; db $00,$01,$02,$03,$04
    exec_start:                    ;
    mov $33,#$44                   ; db $8f,$44,$33
endspcblock execute exec_start     ; dw $0000, $6004  (execution_address)
```
