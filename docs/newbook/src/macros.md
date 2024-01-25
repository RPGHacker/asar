# Macros

Macros are a mechanism for recording a sequence of commands that can be used in other places. The main purpose of this is to include commonly used code in multiple places without actually having to rewrite or copy that code every time. Instead you can move it into a macro and write it only once. Macros, in concept, work similarly to defines in that they are a text-replacement mechanism, but they have a few key differences:

-   Macros only record a sequence of commands rather than any kind of text.
-   A macro call is itself considered a command and thus needs to go on its own line (or separated via the [single-line operator `:`](./formatting.md#single-line-operator)). This means that unlike a define, a macro can't just be used whereever.
-   Macros can include parameters, which are identifiers that are replaced by a text value whenever the respective macro is called. For simplicity, you could consider parameters a macro-specific version of defines.

Use the following syntax to define a macro:

{{# hiddencmd: macro {identifier}(...) #}}{{# hiddencmd: endmacro #}}
```asar
macro {identifier}([parameter1_identifier[, parameter2_identifier...]][variadic_token])
    [command1]
    [command2...]
endmacro
```

where all the identifiers can contain any of the following characters: `a-z A-Z 0-9 _`

Use the syntax `<parameter_identifier>` to expand a parameter inside a macro. This works just like placing a `!define_identifier` anyhwere else in the code. Macros can be recursive (macros calling themselves) and/or nested up to 512 levels deep. This limit only serves the purpose of preventing infinite recursion. The first and last line of the macro definition need to go on their own lines (the [single-line operator](./formatting.md#single-line-operator) is not supported here). To call a macro that has already been defined, use the syntax

```asar
%{identifier}([parameter1[, parameter2...]])
```

where each individual parameter may be wrapped in double quotes (which is required for parameters that contain any whitespace).

```asar
macro mov(target, source)
    lda <source>
    sta <target>
endmacro

macro swap(first, second)
    %mov($00, <first>)
    %mov(<first>, <second>)
    %mov(<second>, $00)
endmacro

macro use_x_safely(code)
    phx
    <code>
    plx
endmacro

%swap($01, $02)
%use_x_safely("ldx $10 : stx $11 : ldx #10 : stx $12")
```

## Variadic Macros

In addition to named substitutions if the variadic token `...` is specified as the last parameter asar will allow an arbitrary number of parameters after all prior parameters have been satisfied. To access unnamed parameters of a variadic macro, use the syntax `<...[{math}]>`, where `math` is any math expression evaluating to the index of a variadic parameter. These are declared numerically starting from 0 up to the number of provided parameters. To access the number of provided variadic arguments one may use `sizeof(...)`. Lastly, it is important to note that while traditional macros do not parse defines inside parameters, variadic macros will. This is to allow iteration of arguments by using defines.

```asar
macro example0(...)
    db sizeof(...), <...[0]>    ;04 01
endmacro
    
macro example1(...)
    !a #= 0
    while !a < sizeof(...)
        db <...[!a]>        ;01 02 03
        !a #= !a+1
    endwhile
endmacro

macro example2(named_parameter, ...)
    !a #= 0
    while !a < sizeof(...)
        db <...[!a]>        ;02 03 04 05 06 07
        !a #= !a+1
    endwhile
    db <named_parameter>    ;01
endmacro

macro macro_with_optional_arguments(required, ...)
    db <required>
    if sizeof(...) > 0
        db <...[0]>
    endif
endmacro

%example0(1,2,3,4)
%example1(1,2,3)
%example2(1,2,3,4,5,6,7)
%macro_with_optional_arguments(1)
%macro_with_optional_arguments(2, 3)
```

## Nested Macro Definitions

Macro definitions can be nested. Doing this, a few special rules come into play. Normally, macro parameters and defines inside macros are only resolved whenever the respective macro is being called, and macros can only resolve their own parameters. This might be impractical once working with nested macro definitions, so Asar provides a special syntax to control the resolution timing of macro parameters and defines. By prepending a number of `^` to the respective name, earlier resolution can be forced, called "backwards-resolution". For example, `<^param>` will be resolved while the parent macro is being called; `!^^define` will be resolved while the grand-parent macro is being called, and so forth.

```asar
!define_01 = $01
!define_02 = $02

macro threefold_one(shadowed)
    db <shadowed> ; Will be resolved when %threefold_one() is called.
    macro threefold_two(not_shadowed)
        !define_01 = $FF ; Will be resolved when %threefold_two() is called.
        !define_02 = $FF ; Will be resolved when %threefold_two() is called.
        db !define_01 ; Will be resolved when %threefold_two() is called.
        db !^define_02 ; Will be resolved when %threefold_one() is called.
        db <^shadowed> ; Will be resolved when %threefold_one() is called.
        db <not_shadowed>
        macro threefold_three(shadowed)
            db <^^shadowed> ; Will be resolved when %threefold_one() is called.
            db <^not_shadowed> ; Will be resolved when %threefold_two() is called.
            db <shadowed> ; Will be resolved when %threefold_three() is called.
        endmacro
    endmacro
endmacro

%threefold_one($03)
%threefold_two($04)
%threefold_three($05)

; Writes: 03 FF 02 03 04 03 04 05
```
