# Math

Math is supported in all opcodes, functions and labels. Asar applies the conventional operator prioritization rules (PEMDAS) in math expressions and supports parentheses for explicit control over the order of operations.

```asar
lda #5+6*2      ; the same as "lda #17"
lda #(5+6)*2    ; the same as "lda #22"
```

Math statements in Asar support the following operators:  
  
| Op | Action                                                                 |
|----|------------------------------------------------------------------------|
| `+`  | Addition (Also valid as prefix, but a no-op)                         |
| `-`  | Subtraction (Or negation prefix)                                     |
| `*`  | Multiplication                                                       |
| `/`  | Division                                                             |
| `%`  | Modulo (the remainder of a division, fmod() in C)                    |
| `<<` | Left-shift ( `x << y` formula: x = x * 2^y )                         |
| `>>` | Right-shift ( `x >> y` formula: x = x / 2^y )                        |
| `&`  | Bitwise AND                                                          |
| <code>\|</code> | Bitwise OR                                                |
| `^`  | Bitwise XOR (Note: not exponentials)                                 |
| `~`  | Bitwise NOT (Prefix)                                                 |
| `<:` | Bitshift right 16, shorthand for isolating address bank (Prefix)     |
| `**` | Exponentials (2\*\*4 = 2\*2\*2\*2 = pow(2, 4) in C)                  |

  
Note that whitespace is not supported inside math statements (ed: in asar 2 it is. kinda.), but the [multi-line operator `\`](./formatting.md#multi-line-operators) can be used to split them into multiple lines. Using math in labels can be useful when you want to apply an offset to the label:

```asar
lda .Data+3    ; Will load $03 into A
        
.Data
    db $00,$01,$02
    db $03,$02,$03
```
