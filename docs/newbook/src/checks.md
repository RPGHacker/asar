# Checks

Checks allow Asar to monitor certain states and throw warnings or errors when certain criteria are met. This can be helpful for catching or preventing certain problems.

## `check title`

{{# syn: check title "{title}" #}}

The check title command verifies that the title stored in the output ROM is identical to `title`. If it isn't, an error is thrown (unless `--no-title-check` is passed to the application, in which case only a warning is thrown - see section [Usage](./usage.md#--no-title-check) for details). The purpose of this command is to assure that patches are applied to the correct output ROM.

```asar
; This patch is only for a Super Mario World ROM
check title "SUPER MARIOWORLD     "

; Remove small bonus stars from game
org $009053
    nop #3
    
org $009068
    nop #3
```

## `check bankcross`

{{# syn: check bankcross {off/half/full} #}}

The `check bankcross` command enables (`full` or `half`) or disables (`off`) throwing errors when a bank border is crossed while assembling a file. The default is `full`, which checks whether the code crosses from pc $FFFF to $0000 in the next bank, and throws an error if that happens. With `half`, Asar will additionally check crossings from $7FFF to $8000. Use `off` with caution as some features may not behave correctly with bank border checking disabled and some places may still check for bank borders, anyways.

```asar
check bankcross off

org $80FFFF

    db $00,$00
    
check bankcross full

print pc    ; Will print 818001 when using LoROM mapper
```
