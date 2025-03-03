# Warnings

Warnings are messages that Asar outputs to inform the user about potentially unintended or risky code that isn't critical and thus doesn't cause assembly to fail on its own. These messages can be useful for detecting potential problems in the code, but in some situations may be undesirable. For this reason, Asar supports a few methods of explicitly enabling or disabling certain warnings (see section [Usage](./usage.md) for details on how to configure warnings via the command line). Additionally, there are warnings which may be useful in some situations, but would be intrusive in most other situations. They are disabled by default and have to be enabled explicitly to be used. Commands that enable or disable warnings refer to them via their names. The easiest way of finding the name of a specific warning is to look at the console output of a patch producing it. Asar will always output the warning name along with the respective warning. A list of all warnings can also be found [here](./warning-list.md).

## Disabled Warnings

This is a list of all warnings that are disabled by default and have to be enabled explicitly.

| Warning name | Details |
| ------------ | ------- |
| `Wimplicitly_sized_immediate` | Thrown when opcodes are sized implicitly and Asar has to assume a size. An opcode is considered to be sized explicitly when either a length specifier is used or a simple hex constant that can be assumed to be of a specific size (that is, a hex constant with either two or four digits). Opcodes that don't support multiple sizes are always considered to be sized explicitly. Everything else is considered to be sized implicitly and will throw this warning when enabled. |
| `Wcheck_memory_file` | Only relevant for the DLL API. Thrown when a file is accessed that was either not provided as a memory file or that isn't found in memory. Mainly intended for debugging purposes and can be used to assure that files are actually read from the correct location. |

## `warnings {push/pull}`

{{# syn: warnings {push/pull} #}}

The `warnings push` command pushes the current state of enabled and disabled warnings to the stack. The `warnings pull` command pulls it back from the stack.

```asar
warnings push
; Disable "freespace leaked" warning
warnings disable Wfreespace_leaked

freecode

; [...]

warnings pull
```

## `warnings {enable/disable}`

{{# syn: warnings {enable/disable} {name} #}}

The `warnings enable` command enables the warning with the specified name, the `warnings disable` command disables it. Warnings enabled or disabled via this command override warnings enabled or disabled via the command line (see section [Usage](./usage.md) for details). When using these commands inside shared code, it's recommended to do so in conjunction with [`warnings {push/pull}`](#warnings-push-pull) to prevent the modified settings from leaking into other files.

```asar
warnings disable Wwarn_command

warn "This text in invisible!"

warn enable Wwarn_command

warn "This text in visible!"
```
