# Compatibility Settings

Compatibility settings determine how Asar operates in certain situations. They can be changed via a number of commands.

## `asar`

{{# syn: asar {ver} #}}

The `asar` command can be used to specify the minimum Asar version your patch is compatible with. The `ver` parameter specifies the minimum required Asar version. When a user tries to assemble the patch in an older version of Asar, an error will be thrown, stating that the used Asar version is too old. This should be the first command in your patch, otherwise an error will be thrown.

```asar
; This patch uses features from Asar 1.40, so it makes sense to require it as a minimum.
asar 1.40

if readfile1("data.bin", 0) == 1
    ; Do something
else
    ; Do something else
endif
```

## `namespace nested`

{{# syn: namespace nested {on/off} #}}

The `namespace nested` command enables (`on`) or disables (`off`) nested namespaces. The default is `off`. See section [Namespaces](./namespaces.md) for details.
