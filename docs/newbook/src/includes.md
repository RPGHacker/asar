# Includes

Includes make it possible for your code to reference other files. This can be done for a number of reasons. The most common scenarios are to split code into multiple source files (see [`incsrc`](#incsrc)) or to separate code from data (see [`incbin`](./binary.md#incbin)). Whenever using a command or function referencing another file, Asar tries to locate that file by applying a set of rules to the file path in a specific order:

1.  If the path is an **absolute** path:
    1.  Asar tries to locate the file directly via the specified path.
    2.  If this fails, an error is thrown.
2.  If the path is a **relative** path:
    1.  Asar tries to locate the file relatively to the file currently being assembled. (Caution: when used inside macros, paths are relative to the macro definition rather than to the macro call).
    2.  If this fails, Asar tries to locate the file relatively to any of the include search paths that were specified, in the order they were specified in, until the file is found. (See section [Usage](./usage.md) for details on include search paths).
    3.  If all of the previous fail, an error is thrown.

Asar supports Unicode file names where available, including on Windows.

## `incsrc`

{{# syn: incsrc {filename} #}}

The incsrc command makes Asar assemble the file referenced by the `filename` parameter (enclose in double quotes to use file names with spaces, see section [Includes](#includes) for details on Asar's handling of file names). The file is assembled in-place which means that Asar instantly switches to the new file and only returns to the previous file once assembling the new file has finished. All of Asar's state (labels, defines, functions, pc etc.) is shared between files. When including other files, there is a recursion limit of 512 levels. This limit only serves the purpose of preventing infinite recursion. For an easier understanding of incsrc, you can visualize it as a command which pastes the contents of another file directly into the current file (although that's not actually how it's implemented and there are differences in the way relative file paths are handled).

```asar
; Contents of routine.asm:
;AnotherRoutine:
;    lda #$FF
;    sta $00
;    rts

Main:
    jsr AnotherRoutine
    bra Main

incsrc "routine.asm"
        
```

## `include` / `includefrom`

{{# syn: include #}}
{{# syn: includefrom {filename} #}}

The `include` and `includefrom` commands specify that a file is only to be included in another file and not to be assembled directly. When a user tries to assemble a file containing include or includefrom directly, an error is thrown. The `includefrom` command behaves identically to the include command with the exception that it is passed the name of the file it is meant to be included from (note that Asar doesn't verify whether it's actually included from that file, it only checks whether it's included from another file at all). When making use of include or includefrom, they must be the first command within their respective file and can't be used in combination with the [asar](./compat.md#asar) command in the same file.

```asar
; Contents of shared.asm:
;includefrom main.asm
;
;if read1($00FFD5) == $23
;    !is_sa1_rom = 1
;else
;    !is_sa1_rom = 0
;endif


asar 1.37

incsrc "shared.asm"

if !is_sa1_rom != 0
    ; ...
endif
```

## `includeonce`

{{# syn: includeonce #}}

The includeonce command places an include guard on the file that is currently being assembled. This prevents it from being assembled again when included again. This is intended for shared files which may be included from multiple source files, but should only be assembled once to prevent redefinition errors etc.

```asar
; Contents of shared.asm:
;
;includeonce
;
;MyRoutine = $018000
;MyOtherRoutine = $028000


; Note that the second include does not throw
; redefinition errors, thanks to the "includeonce".
incsrc "shared.asm"
incsrc "shared.asm"

jsl MyRoutine
jsl MyOtherRoutine
```
