# Code Formatting and Syntax

## Encoding

Asar expects all source files to be UTF-8-encoded and will throw an error when detecting any other encoding.

## Comments

You can use `;` to add comments to your code, making it easier to read and understand for other people. Everything from the `;` to the end of the line is silently ignored by Asar.

```asar
    lda $00        ; Asar only sees the lda $00 and ignores everything else
```

## Brackets

Brackets, {{# cmd: { #}} and {{# cmd: } #}}, may be used to help organize your code structurally. They're treated as commands by the assembler, which means they follow the same rules as other commands, but they otherwise have no effect on code assembly and are silently ignored. Since brackets have no effect on code assembly, they don't even have to match, either. It's entirely up to the coder whether, how and in what quantity brackets are used.  

```asar
    lda $00
    beq .IsZero
    
.GreaterThanZero
    {
        dec $00
    }

.IsZero
    rts
```

## Multi-Line Operators

The `,` and the `\` operator are formatting operators which make it possible to split commands in Asar into multiple lines. Both are put at the end of a line and work very similarly with only one key difference. During execution, Asar will concatenate subsequent lines to lines ending with either operator and treat them as a single line. When using the comma operator, the comma itself will actually remain a part of the concatenated string, whereas when using the backslash operator, the backslash itself will be removed from the concatenated string. When using the backslash operator, please note that all whitespace following it is ignored, whereas all whitespace preceeding it is preserved. This is by design, since some commands in Asar require spaces to work, whereas other commands (like math commands) only work without spaces.

```asar
%some_macro(!arg1, !arg2, !arg3,
    !arg4, !arg5, !arg6)
; This will be treated as "%some_macro(!arg1, !arg2, !arg3, !arg4, !arg5, !arg6)"

lda \
    $7F0000
; This will be treated as "lda $7F0000"

function func(param) = ((param*param)+1000)\
    /256
; This will be treated as "function func(param) = ((param*param)+1000)/256"
```

## Single-Line Operator

Contrary to the multi-line operators, the single-line operator `:` is a formatting operator which makes it possible to treat a single line of code as multiple lines. It requires a space before and after usage to differentiate it from the : used with certain commands. When used between different commands, Asar interprets it similarly to a new line and treats each command as being on a separate line. This can be used to link multiple commands together into functional blocks and make the code more readable.

```asar
lda #$00 : sta $00
        
; Treated as:
lda #00
sta $00
```
