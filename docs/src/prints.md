# Text Output

Text output functions allow you to communicate certain information, states, warnings, errors etc. to end users of your code.

## `print`

The `print` command lets you output general-purpose text to the user. Most commonly this is used to inform the user about certain states or to output debug information. Usage:

{{# syn: print {text_or_function}[,text_or_function...] #}}

where `text_or_function` can be either a string delimited by double quotes or one of the print-specific functions below:  
  

| Function | Details |
| --- | --- |
| `bin(x[, width])` | Prints x as a binary (base-2) integer, where x can be any math statement. If width is provided, the output is padded to at least this many digits using zeroes. |
| `dec(x[, width])` | Prints x as a decimal (base-10) integer, where x can be any math statement. If width is provided, the output is padded to at least this many digits using zeroes. |
| `hex(x[, width])` | Prints x as a hexadecimal (base-16) integer, where x can be any math statement. If width is provided, the output is padded to at least this many digits using zeroes. |
| `double(x[, precision])` | Prints x as a decimal number with precision decimal places (default: 5), where x can be any math statement. |
| `pc` | Prints the current PC. |
| `freespaceuse` | Prints the total number of bytes used by commands that acquire freespace (such as freespace, freecode, freedata etc.). You can use the command {{# cmd: reset freespaceuse #}} to reset this value. |
| `bytes` | Prints the total number of bytes written to the output ROM. You can use the command {{# cmd: reset bytes #}} to reset this value. |

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
