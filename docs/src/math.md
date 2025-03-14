# Math

Math is supported in all opcodes, functions and labels. Asar applies the conventional operator prioritization rules (PEMDAS) in math expressions and supports parentheses for explicit control over the order of operations.

```asar
lda #5+6*2      ; the same as "lda #17"
lda #(5+6)*2    ; the same as "lda #22"
```

## Literals

Asar supports decimal, hexadecimal and binary number literals. Hexadecimal literals use `$` as a prefix, binary literals use `%` as a prefix. Number literals can be made positive or negative by prefixing a `+` or a `-` (without a sign, positive is assumed). They can also be prefixed with a `~` to get their unary complement (an integer with all the bits inverted).

```asar
lda $00
clc
adc #-10
and #%01111111
lda #~$80   ; Equal to lda #$7F
```

Aditionally, Asar supports character literals by delimiting a single Unicode character with `'`. Asar will automatically convert them to the integer value currently mapped to them (by default their Unicode code point). They can be used in all places where number literals can be used. See section [Tables](./binary.md#tables) for details on character mapping.

```asar
lda #'a'
sta $00

db 'x','x'+1,'x'+2

db '💩'
```


## Operators

TODO: document operator precedence

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

## Comparison operators

Asar supports the 6 usual comparison operators:

| Operator | Details |
| --- | --- |
| `a == b` | Returns 1 if `a` is equal to `b` |
| `a != b` | Returns 1 if `a` is not equal to `b` |
| `a > b` | Returns 1 if `a` is greater than `b` |
| `a < b` | Returns 1 if `a` is less than `b` |
| `a >= b` | Returns 1 if `a` is greater than or equal to `b` |
| `a <= b` | Returns 1 if `a` is less than or equal to `b` |

## Logical operators

| Operator | Details |
| --- | --- |
| <code>a \|\| b</code> | Returns 1 if at least one of `a` and `b` evaluates to true |
| `a && b` | Returns 1 if both of `a` and `b` evaluate to true |

These operators are lazy: they will not evaluate the right-hand argument if the result is already determined by the left-hand argument. (Specifically, `1 || anything` immediately returns `1` and doesn't evaluate `anything`, and similarly, `0 && anything` immediately returns `0`.)

## Strings in math

Strings are allowed in math, though the only operator that can use them is `+`, which concatenates its arguments. Strings can, however, be used in both built-in and user-defined functions.

The comparison operators are allowed on strings, they compare the strings lexicographically. Equality between strings and numbers is always false, comparing strings with numbers using `>`/`<`/etc is an error.

For the purpose of logical operators and conditional commands, only the empty string is considered false, all other strings are true.
