# Structs
Structs are an advanced form of labels with the purpose of making access into structured data blocks easier. The general syntax is as follows

{{# hiddencmd: struct {identifier} {snes_address} #}}{{# hiddencmd: endstruct [align {num}] #}}
```asar
struct {identifier} {snes_address}
    [label...]
endstruct [align {num}]
```

where `identifier` can contain any of the following characters:  
`a-z A-Z 0-9 _`  
The `snes_address` parameter can be any number literal or math statement evaluating to an SNES address. This address marks the start of the struct. The `label` parameter should be any number of labels, ideally coupled with skip commands. These labels become offsets into the struct. Internally, the struct command will do something similar to this

```asar
pushpc
base snes_address
```

whereas the endstruct command will do something similar to this

```asar
base off
pullpc
```

Take a look at the simple example below:

```asar
struct ObjectList $7E0100
    .Type: skip 1
    .PosX: skip 2
    .PosY: skip 2
    .SizeX: skip 1
    .SizeY: skip 1
endstruct
```

This defines a struct called `ObjectList` at location `$7E0100` with a size of `7` (the sum of all skip commands). You can access into this struct like so:

```asar
lda ObjectList.PosY
```

This is equal to:

```asar
lda $7E0103     ; $7E0100+1+2
```

The final address is calculated by taking the start of the struct (`$7E0100`) and adding to that all the skips preceding the `.PosY` label (`1` and `2`). Aside from accessing structs directly, it's also possible to access them as arrays. A simple example:

```asar
lda ObjectList[2].PosY
```

The final address in this case is calculated by the equation:  
`struct_start + (array_index * struct_size) + label_offset`  
So in this case, our final address is `$7E0100 + (2 * 7) + (1 + 2) = $7E0111`. When using structs this way, the optional `align` parameter becomes relevant. This parameter controls the struct's alignment. Simply put, when setting a struct's alignment, Asar makes sure that its size is always a multiple of that alignment, increasing the size as necessary to make it a multiple. Let's take another look at the example above with an added alignment:

```asar
struct ObjectList $7E0100
    .Type: skip 1
    .PosX: skip 2
    .PosY: skip 2
    .SizeX: skip 1
    .SizeY: skip 1
endstruct align 16
```

With an alignment of 16 enforced, this struct's size becomes 16 (the first multiple of 16 that 7 bytes fit into). So when accessing the struct like this

```asar
lda ObjectList[2].PosY
```

the final address becomes `$7E0100 + (2 * 16) + (1 + 2) = $7E0123`. If we add some data into the struct

```asar
struct ObjectList $7E0100
    .Type: skip 1
    .PosX: skip 2
    .PosY: skip 2
    .SizeX: skip 1
    .SizeY: skip 1
    .Properties: skip 10
endstruct align 16
```

its original size becomes 17. Since a final size of 16 would now be too small to contain the entire struct, the alignment instead makes the struct's final size become 32 (the first multiple of 16 that 17 bytes fit into), so in our example of

```asar
lda ObjectList[2].PosY
```

we now end up with a final address of `$7E0100 + (2 * 32) + (1 + 2) = $7E0143`.  

## Extending structs

Another feature that is unique to structs is the possibility of extending previously defined structs with new data. The general syntax for this is as follows:

{{# hiddencmd: struct {identifier} extends {identifier} #}}
```asar
struct {extension_identifier} extends {parent_identifier}
    [label...]
endstruct [align {num}]
```

This adds the struct `extension_identifier` at the end of the previously defined struct `parent_identifier`. Consider the following example:

```asar
struct ObjectList $7E0100
    .Type: skip 1
    .PosX: skip 2
    .PosY: skip 2
    .SizeX: skip 1
    .SizeY: skip 1
endstruct

struct Properties extends ObjectList
    .Palette: skip 1
    .TileNumber: skip 2
    .FlipX: skip 1
    .FlipY: skip 1
endstruct
```

The struct `ObjectList` now contains a child struct `Properties` which can be accessed like so:

```asar
lda ObjectList.Properties.FlipX
```

Since extension structs are added at the end of their parent structs, the offset of `.FlipX` in this example is calculated as  
`parent_struct_start_address + parent_struct_size + extension_struct_label_offset`,  
in other words, our final address is `$7E0100 + 7 + (1 + 2) = $7E0109`. Note that extending a struct also changes its size, so in this example, the final size of the `ObjectList` struct becomes 12. Extended structs can also be accessed as arrays. This works on the parent struct, as well as the extension struct.

```asar
lda ObjectList[2].Properties.FlipX
```

```asar
lda ObjectList.Properties[2].FlipX
```

In the first example, our final address is calculated as  
`parent_struct_start_address + (combined_struct_size * array_index) + parent_struct_size + extension_struct_label_offset`,  
whereas in the second example, it's calculated as  
`parent_struct_start_address + parent_struct_size + (extension_struct_size * array_index) + extension_struct_label_offset`,  
so we end up with final addresses of `$7E0100 + (12 * 2) + 7 + (1 + 2) = $7E0122` and `$7E0100 + 7 + (5 * 2) + (1 + 2) = $7E0114`.  
  
A few further things to note when using structs in Asar:

<!-- TODO: the first point sounds like the exact use case of extension structs tho?? should re-word this -->
-   It's possible to extend a single struct with multiple extension structs. However, this can be counter-intuitive. The size of the extended struct becomes the size of the parent struct plus the size of its largest extension struct, rather than the size of the parent struct plus the sizes of each of its extension structs. This also means that when accessing those extension structs, they all start at the same offset relative to the parent struct. This can be confusing and is often not what's actually intended, so for code clarity, it's recommended to only extend structs with at most a single other struct.
-   It's possible to enforce alignments when using extension structs. However, this will only determine the alignment of the parent struct and/or the extension struct(s), depending on where it's specified. It won't determine the alignment of the combined struct. This can be confusing and is usually not what is intended. There currently is no universal workaround for this, so when a certain alignment is required for a struct, it's recommended to not use extension structs with it.
-   It's not possible to access both, a parent struct and its extension struct, as arrays simultanously.
-   An extension struct can't be extended itself.
