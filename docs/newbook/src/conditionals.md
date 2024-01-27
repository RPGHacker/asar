# Conditionals and Loops
Conditional compilation allows you to only compile specific sections of code when certain conditions are met. This can be used in a number of ways, but is most commonly used in conjunction with defines to make code easily customizable and/or provide some simple configuration options to end users.

## `if` / `elseif` / `else` / `endif`

The most basic form of conditionals are if conditionals. They are given a math statement and only compile their enclosed code if that statement evaluates to a value greater than 0.

{{# hiddencmd: if {condition} #}}
{{# hiddencmd: elseif {condition} #}}
{{# hiddencmd: else #}}
{{# hiddencmd: endif #}}
```asar
if {condition}
    {codeblock}
endif
```

To construct condition statements, you can also make use of a number of comparison operators specific to conditionals. They return 1 if their respective comparison is true and 0 otherwise.  
  
| Operator | Details |
| --- | --- |
| `a == b` | Returns 1 if `a` is equal to `b` |
| `a != b` | Returns 1 if `a` is not equal to `b` |
| `a > b` | Returns 1 if `a` is greater than `b` |
| `a < b` | Returns 1 if `a` is less than `b` |
| `a >= b` | Returns 1 if `a` is greater than or equal to `b` |
| `a <= b` | Returns 1 if `a` is less than or equal to `b` |
  
You can use logical operators to chain multiple conditions.  
  
| Operator | Details |
| --- | --- |
| <code>a \|\| b</code> | Returns 1 if at least one of `a` and `b` evaluates to 1 |
| `a && b` | Returns 1 if both of `a` and `b` evaluate to 1 |

Evaluation is lazy (TODO it's not anymore, is it?) which means that the assembler will stop evaluating a condition as soon as the result can be determined (for example, in the condition `0 && my_function()`, my\_function() will never be called). Note that only one kind of logical operator can be used in a single condition, but conditionals themselves can be nested to an arbitrary depth, which can be used as a workaround here.

Optionally, if conditionals can contain an arbitrary number of elseif branches as well as a single else branch. The assembler checks the if and all elseif branches in succession until a single condition evaluates to `> 0` - if none does, the code inside the else branch is compiled.

```asar
!mode = 0       ; Supported modes: 0, 1, 2, 3
!verbose = 0    ; Set to 1 to enable verbose mode

if !mode == 0
    ; ...
elseif !mode == 1
    ; ...
elseif !mode == 2
    ; ...
elseif !mode == 3
    if !verbose != 0
        print "Oh boy, so you're going with mode 3 today!"
    endif
    ; ...
else
    error "Unsupported mode! Please choose 0, 1, 2 or 3!"
endif
```

Alternatively, if statements can also be constructed on a single line via the following syntax:

```asar
if {condition} : {codeblock}[ : codeblock...] : endif
```

```asar
PressedY:
    if !fireballs_enabled : %PlaySoundEffect(!fireball_sfx) : jsr ShootFireball : endif
    rtl
```

If you plan to use labels in if conditions, note that there's certain restrictions that apply. More specifically, only static labels can be used. That is, only labels whose address can't change between Asar's passes, as demonstrated by the following example:

```asar
FirstLabel = $018000
        
freecode
    lda SecondLabel,x
    
SecondLabel:
    db $00,$01,$02,$03
    
; All good. FirstLabel was statically defined.
if FirstLabel == 0
endif

; Error Elabel_in_conditional. The label could move between passes.
if SecondLabel == 0
endif
```

## `while`

{{# hiddencmd: while {condition} #}}
{{# hiddencmd: endwhile #}}
```asar
while {condition}
    {code}
endwhile
```

A special variation of if conditionals are while loops. Instead of compiling their enclosed code only once, they compile it repeatedly until their condition evaluates to `<= 0`. Typically, this would be used with a define that is modified inside the loop. This can be useful for generating data tables.

```asar
!counter = 0
        
while !counter < $10
    db (!counter<<8)|$00,(!counter<<8)|$01,(!counter<<8)|$02,(!counter<<8)|$03
    db (!counter<<8)|$04,(!counter<<8)|$05,(!counter<<8)|$06,(!counter<<8)|$07
    db (!counter<<8)|$08,(!counter<<8)|$09,(!counter<<8)|$0A,(!counter<<8)|$0B
    db (!counter<<8)|$0C,(!counter<<8)|$0D,(!counter<<8)|$0E,(!counter<<8)|$0F
            
    !counter #= !counter+1
endwhile
```

Be warned as improper use of while loops can lead to infinite loops and thus a dead-lock of the assembler, as Asar won't attempt to detect those.

## `for`

{{# hiddencmd: for {variable} = {start}..{end} #}}
{{# hiddencmd: endfor #}}
```asar
for {variable} = {start}..{end}
    {code}
endfor
```

For loops repeat the contents a specified number of times. In the for loop body, you have access to a loop counter as a define. The range is specified as start-inclusive and end-exclusive. For example:

```asar
for i = 1..5
    db !i
    db 2*!i
endfor
```

This will write the bytes `01 02 02 04 03 06 04 08`.

You can also put for loops on a single line, however in this case due to the order in which Asar parses defines, you will not be able to use the loop counter. For example:

```asar
for i = 0..10 : nop : endfor
```
