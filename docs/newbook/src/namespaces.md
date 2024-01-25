# Namespaces

{{# syn:
namespace {identifier}
namespace off
#}}

Namespaces are a feature which makes it easier to avoid name conflicts between different labels without having to give them long or cryptic names. They work similarly to C++ namespaces and accomplish this by automatically adding a prefix to all labels declared or accessed within them. This prefix consists of an identifier, followed by an underscore `_` . Namespaces can be stacked if desired by enabling the [`namespace nested`](./compat.md#namespace-nested) setting. When you try to access a label from within a namespace and Asar doesn't find it in there, it automatically looks in the upper namespaces (up to the global namespace).

Use `namespace {identifier}` to enter a namespace, where `identifier` can contain any of the following characters: `a-z A-Z 0-9 _`

Use `namespace off` to leave the current namespace (or immediately return to the global namespace when nested namespaces are not enabled).

```asar
; All of the below is valid

namespace nested on

Main:                           ; Main
Main2:                          ; Main2

namespace Deep

    Main:                       ; Deep_Main
    
    namespace Deeper
    
        Main:                   ; Deep_Deeper_Main
        Main3:                  ; Deep_Deeper_Main3
        
        namespace Deepest
            
            Main:               ; Deep_Deeper_Deepest_Main
            
            dl Main             ; Deep_Deeper_Deepest_Main
            dl Main2            ; Main2
            dl Main3            ; Deep_Deeper_Main3
            
        namespace off
            
        dl Main                 ; Deep_Deeper_Main

    namespace off
            
    dl Main                     ; Deep_Main
    
namespace off


namespace nested off

namespace TheFirst

    Main:                       ; TheFirst_Main
    
    dl Main                     ; TheFirst_Main
    
namespace TheSecond

    Main:                       ; TheSecond_Main
    
    dl Main                     ; TheSecond_Main
    
namespace TheThird

    Main:                       ; TheThird_Main
    
    dl Main                     ; TheThird_Main
    
namespace off


dl Main                         ; Main
dl Deep_Main                    ; Deep_Main
dl Deep_Deeper_Main             ; Deep_Deeper_Main
dl Deep_Deeper_Deepest_Main     ; Deep_Deeper_Deepest_Main

dl TheFirst_Main                ; TheFirst_Main
dl TheSecond_Main               ; TheSecond_Main
dl TheThird_Main                ; TheThird_Main
```

## `pushns` / `pullns`

{{# cmd: pushns #}} saves the current namespace. {{# cmd: pullns #}} restores the last-pushed value of the namespace.

## Global labels

While in a namespace, you can use the keyword `global` to define labels outside all namespaces. The syntax is {{# cmd: global [#]{identifier}: #}}. For example:

```asar

namespace NS
global GlobalLabel:
.Sub: ; this is a sublabel of GlobalLabel

LocalLabel:

global #AnotherGlobal: ; this global won't modify the sublabel hierarchy

.Sub: ; this is a sublabel of LocalLabel
namespace off

; these are all valid:
dl NS_LocalLabel
dl NS_LocalLabel_Sub
dl GlobalLabel
dl GlobalLabel_Sub
dl AnotherGlobal

```

Note that `#` acts the same way as it does for regular labels. Note that you cannot use the `global` command with sublabels or macro labels. Outside of a namespace, `global` acts just like a regular label definition.
