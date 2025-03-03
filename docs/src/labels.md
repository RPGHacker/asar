# Labels

Labels are used to represent a position in the ROM and allow you to code without having to constantly update branches and jumps/calls. They can be used with any opcode, but were specifically designed to be used with branches, jumps, calls, pointer tables etc. When used with branches, they're automatically converted to offsets.

## Main Labels

```asar
[#]{identifier}:
```

Main labels are the top-most level of labels supported by Asar. They're global and thus can be directly acessed from anywhere. Their identifier can contain any of the following characters: `a-z A-Z 0-9 _`

```asar
org $008000

Main:
    %do_frame()
    jmp Main    ; Equal to jmp $8000
```

An alternate form of defining main labels is by directly assigning a value to them. A common use-case for this is to make a label point to an existing address inside a ROM. Syntax:

```asar
{identifier} = {snes_address}
```

where `snes_address` can be a number or any math statement evaluating to an SNES address. Note that defining a main label this way does not start a new [sub label](#sub-labels) group.

```asar
Main:
; ...

SomewhereInRom = $04CA40

.Sub:
; ...

Table:
    dl Main_Sub                 ; Okay!
    dl SomewhereInRom_Sub       ; Error, label not found
```

Prefixing a label definition (except label assignments) with a `#` will define the label without modifying existing label hierarchies. This can be useful for defining global routines inside call-anywhere macros without having them break existing label hierarchies.

```asar
macro my_new_routine()
    jsl MyNewRoutine
            
    !macro_routine_defined ?= 0
    
    if !macro_routine_defined == 0
        pushpc
        
        freecode cleaned
        
        #MyNewRoutine:
            incsrc routines/mynewroutine.asm
        
        pullpc
    
        !macro_routine_defined = 1
    endif
endmacro

Main:
    %my_new_routine()
.Sub

    ; Both of these are found
    dl MyNewRoutine
    dl Main_Sub
```

Asar includes a label optimizer which attempts to optimize performance by shortening opcodes accessing labels from 24-bit to 16-bit whenever possible. See section [Program Counter](./program-counter.md#optimize-address) for details.

## Sub Labels

```asar
[#].{identifier}[:]
```

Sub labels are the second-most level of labels supported by Asar. They're local to the last main label declared and their identifiers can contain the same characters as main labels.

```asar
Proc1:
    nop
.Sub
    bra .Sub

Proc2:
    nop
.Sub:   ; Note that the colon is optional
    bra .Sub
```

Sub labels allow you to reuse redundantly named labels such as Loop, End, etc. without causing label redefinition errors. A new sub label group is automatically started after a main label is declared. Internally, sub labels are converted to `MainLabel_SubLabel`, which can be used to access them from anywhere.

```asar
Main1:
    ; ...
.Sub1:
    ; ...
.Sub2:
    ; ...
    
Main2:
    ; ...
.Sub1:
    ; ...
.Sub2:
    ; ...
    
Table:
    dl Main1_Sub1
    dl Main1_Sub2
    dl Main2_Sub1
    dl Main2_Sub2
```

Sub labels can themselves contain sub labels to an arbitrary depth by prepending additional dots.

```asar
Main1:
; ...
.Sub:
; ...
..Deeper:
; ...
...TheEnd:
; ...

Table:
    dl Main1_Sub_Deeper_TheEnd
```

Prefixing a sub label definition with a `#` will define the sub label without modifying existing label hierarchies, but there is probably no practical use for this and it's unintuitive, so it should be avoided.

## +/- Labels

```asar
+[+...][:]

-[-...][:]
```

+/- labels are a special type of labels that are different from both main labels and sub labels in that they don't refer to a specific location in code, but rather to a location relative from where they are used. When used inside opcodes etc., `+` always refers to the next + label and `-` always refers to the previous - label. You can also chain an arbitrary number of + or an arbitrary number of - to create unique +/- labels that don't overwrite labels with a different number of +/-, for example `+++` or `-----`.

```asar
    ldx.b #4
        
--              ; A
    lda $10,x
    beq +       ; Branches to "C"
        
    ldy.b #8
    
-               ; B
    %do_something()
    
    dey
    bne -       ; Branches to "B"
    
+:              ; C - note that +/- labels can also include an optional colon in their declaration
    dex
    bpl --      ; Branches to "A"
```

+/- labels are useful in a number of situations. For example: inside a long routine with multiple short loops, where even a sub label like `.Loop` would get repetitive. +/- labels aren't bound to any scope, which means they can technically be used across different scopes. Just like sub labels, +/- labels are converted to main labels internally. Unlike sub labels, they can not be referenced from code directly since their names depend on where in the code they're used, making it impractical to directly refer to them. This is by design. They can, however, be accessed via the Asar DLL API, and their full name may appear in error messages printed by Asar. The naming format used for them is `:pos_x_y` for + labels and `:neg_x_y` for - labels, where `x` = number of chained +/- and `y` = instance of this label within all +/- labels of the same name (starting from 0 for + labels and from 1 for - labels).

```asar
lorom
org $008000

---         ; :neg_3_1
-           ; :neg_1_1
    bra -
--          ; :neg_2_1
-           ; :neg_1_2
    bra ---
    bra --
    bra -
    
    bra ++
    bra +
    bra +++
    
++          ; :pos_2_0
+           ; :pos_1_0
    bra ++
++          ; :pos_2_1
+++         ; :pos_3_0
```

## Macro Labels

```asar
[#]?{identifier}:

?{identifier} = {snes_address}

[#]?.{identifier}[:]

?+[+...]

?-[-...]
```

Macro labels are special variations of the labels mentioned in the previous sections. Functionally, they behave the same as the other labels with the exception of being local to the macro they're used in. This means they can't be referenced from outside the respective macro. Macro labels are created by prefixing any of the other label types with a `?` .

```asar
macro do_something()
    ?MacroMainLabel:
    ?.MacroSubLabel
    ?-
        ; All of these are fine!
        dl ?MacroMainLabel
        dl ?.MacroSubLabel
        dl ?-
        dl ?+
        dl ?MacroMainLabel_MacroSubLabel
    ?+
endmacro

%do_something()

; ERROR! ?MacroMainLabel is undefined, because we're not inside %do_something() anymore.
dl ?MacroMainLabel
```

Prefixing a macro label definition (except for macro label assignments and macro +/- labels) with a `#` will define the macro label without modifying existing label hierarchies, but there is probably no practical use for this, so it should be avoided. Like all other labels, macro labels are converted to main labels internally and prefixed with an identifier of `:macro_x_` where `x` = total macro call count. They can't be referenced in code directly, except inside their respective macro and using the respective macro label syntax seen above. They can, however, be accessed via the Asar DLL API, and their full name may appear in error messages printed by Asar.
