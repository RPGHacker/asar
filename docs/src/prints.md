# Text Output

Text output functions allow you to communicate certain information, states, warnings, errors etc. to end users of your code.

## `print`

The `print` command lets you output general-purpose text to the user. Most commonly this is used to inform the user about certain states or to output debug information. Usage:

{{# syn: print {text_or_math}[,text_or_math...] #}}

where `text_or_math` can either be a string literal in double-quotes, a math expression that evaluates to a string (see [String formatting functions](./functions.md#string-formatting-functions)), or one of the following special values:
  

| Value | Details |
| --- | --- |
| `pc` | Prints the current PC, as an unprefixed 6-digit hexadecimal number. |
| `freespaceuse` | Prints the total number of bytes used by commands that acquire freespace (such as freespace, freecode, freedata etc.), in decimal. You can use the command {{# cmd: reset freespaceuse #}} to reset this value. |
| `bytes` | Prints the total number of bytes written to the output ROM, in decimal. You can use the command {{# cmd: reset bytes #}} to reset this value. |

## `warn`

The `warn` command lets you output a warning message to the user. Usage:

{{# syn: warn [text_or_function...] #}}

where `custom_warning_text` can be a custom warning text and uses the same format as the `print` command. A warning does not cause compilation to fail, so it can be used to inform the user about potential dangers. Warning messages are printed to stderr.

```asar
if read1($00FFD5) == $23
    warn "SA-1 compatibility of this patch is untested, use with caution!"
endif
```

## `error`

The `error` command lets you output an error message to the user. Usage:

{{# syn: error [text_or_function...] #}}

where `custom_error_text` can be a custom error text and uses the same format as the `print` command. An error causes compilation to fail, so it should be used to inform the user about irrecoverable error states. Error messages are printed to stderr.

```asar
if read1($00FFD5) == $23
    error "This patch is not SA-1 compatible!"
endif
```

## `assert`

An assert can be considered a short version of the code

```asar
if {condition}
else
    error [text_or_function...]
endif
```

and is used via the syntax

{{# syn: assert {condition}[,text_or_function...] #}}

where `custom_error_text` can be a custom error text and uses the same format as the `print` command. If `condition` evaluates to `<= 0`, an error is thrown, otherwise nothing happens.

```asar
assert read1($00FFD5) != $23, "This patch is not SA-1 compatible!"
```
