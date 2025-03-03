# Defines

Asar supports a define system that functions similarly to defines in other programming languages, such as C++. Defines are identifiers that you can assign any kind of text to and use in other places as substitues for that text. During compilation, Asar replaces each define it encounters with the respective text assigned to it. Defines are prefixed with a `!` and declared as follows:  

{{# syn:
!{identifier} = {value}
!{identifier} = "{value}"
#}}

where `identifier` is a unique identifier that can contain any of the following characters: `a-z A-Z 0-9 _`

The space on both sides of the = is required, which means that `!identifier=value` will not work. Since defines are really just placeholders for text, they can contain anything - labels, math formulas, even other defines.

```asar
!x = $00
        
lda !x        ; Treated as "lda $00"
lda #!x       ; Treated as "lda #$00"
lda [!x],y    ; Treated as "lda [$00],y"

!y = 34
!x = $12!y
        
lda !x        ; Treated as "lda $1234"
        
!phr = "pha : phx : phy"
        
!phr          ; Treated as "pha : phx : phy"
```

To assign text containing whitespace to a define, you must delimit it with two `"` as shown above with !phr. Besides the regular define operator `=`, Asar also supports a number of additional define operators with slightly different functionality.

| Operator | Functionality |
| --- | --- |
| `=` | The standard define operator. Directly assigns text to a define. | 
| `+=` | Appends text to the current value of a define. | 
| `:=` | Equal to the standard `=`, but resolves all defines in the text to assign before actually assigning it. This makes recursive defines possible. | 
| `#=` | Evalutes the text as though it was a math expression, calculates its result and assigns it to the define. The math is done in-place on the same line the operator is used on. | 
| `?=` | Equal to the standard `=`, but only assigns text to a define that doesn't exist yet, otherwise does nothing. | 

```asar
!define = 10
!anotherdefine = !define+1
; !define now contains "10" and !anotherdefine now contains "!define+1"
!define = 10
!define += 1
; !define now contains "101"
!define = 10
!define := !define+1
; !define now contains "10+1"
!define = 10
!anotherdefine #= !define+1
; !anotherdefine now contains "11"
!lastdefine ?= 10
!lastdefine ?= 1
; !lastdefine now contains "10"
```

  
Similarly to C's ifdef and undef, Asar allows you to check for a define's existence using the `defined("{identifier}")` function and to delete a define using the {{# cmd: undef "{identifier}" #}} command. Make sure to leave the `!` out of the identifier when using these functions, as Asar would otherwise try to resolve the defines.

```asar
!define = "hello"
        
if defined("define")
    print "This will be printed!"
endif

undef "define"
        
if defined("define")
    print "This won't be printed!"
endif
```

Note that Asar tries to replace defines wherever possible, even inside strings. In some occasions, this might be undesirable. See section [Tables](./binary.md) for details on how to escape certain characters.

## Nested Defines

By default, the define parser in Asar considers every supported character in a connected string to be a part of the define's name. This may not always be desired as it can lead to a certain define becoming inaccessible in a certain situation. In cases like that, the `{}` operator makes it possible to still use those defines by resovling everything inside the braces immediately.

```asar
!hex = $
        
db !hexFF     ; Error - define !hexFF not found
db !{hex}FF   ; OK
```

Perhaps the more useful feature of this operator is that it can also be nested to allow for the creation of dynamic define names.

```asar
; Please specifiy a mode from 0 to 3
!mode = 1

assert !mode >= 0 && !mode <= 3, "Please specify a mode from 0 to 3!"

!modename0 = "Default"
!modename1 = "Debug"
!modename2 = "Fast"
!modename3 = "Small"

!modenamestring = !{modename!{mode}}

print "Building in mode: !modenamestring"
```

## Built-in Defines

Aside from user defines, Asar also supports a number of built-in defines. These defines are read-only and any attempt to modify them will throw an error.

| Define | Details |
| ------ | ------- |
| `!assembler` | Contains the value `asar`. Theoretically can be used to differentiate between different assemblers if other assemblers use this define and a syntax similar to Asar. |
| <span style="white-space:pre">`!assembler_ver`</span> | Contains the version number of Asar in the format `(major_version * 10000) + (minor_version * 100) + revision`. For Asar version 1.60, this contains 10600. |
| <span style="white-space:pre">`!assembler_time`</span> | Contains the current Unix timestamp as an integer (number of seconds since 1970-01-01 00:00:00 UTC). |

```asar
if not(stringsequal("!assembler", "asar"))
    warn "This patch was written for Asar and may not be compatible with your current assembler."
endif
if !assembler_ver < 10600
    warn "This patch might not behave correctly due to a bug in Asar versions prior to 1.60."
endif
```
