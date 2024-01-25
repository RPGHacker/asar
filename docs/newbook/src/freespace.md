# Freespace

Freespace is a concept that comes into play when extending an existing ROM. To insert new code or data into a ROM, the ROM must contain enough continuous unused space for everything to fit into. Space like that is referred to as freespace. Many tools attempt to find freespace in a ROM by looking for continuous blocks of a certain value (most commonly `$00`). This method on its own isn't reliable as freespace finders could erroneously detect binary data or code with a certain pattern as freespace. For this reason, the RATS format was invented to protect data inserted into a ROM (see [SMW Wiki](https://web.archive.org/web/20180525193101/http://old.smwiki.net/wiki/ROM_Allocation_Tag_System) for details on the RATS format). When placing RATS tags at the beginning of occupied memory blocks inside a ROM, freespace finders can search for them to know which parts of the ROM not to overwrite. Asar supports a number of commands for working with freespace directly, including freespace finders with automatic RATS tag generation.

## `freespace` / `freecode` / `freedata`

{{# syn:
freespace {ram/noram}[,align][,cleaned][,static][,value]
freecode [align][,cleaned][,static][,value]
freedata [align][,cleaned][,static][,value]
#}}

The freespace command makes Asar search the output ROM for a freespace area large enough to contain the following section of code/data. If such an area is found, the pc is placed at its beginning and a RATS tag automatically written. If no such area is found, an error is thrown. The parameters control what kind of freespace to look for.  
  
| Parameter | Details |
| --- | --- |
| `ram` | The freespace finder searches for an area where RAM mirrors are available (banks $10 to $3F). Recommended when inserting code. |
| `noram` | The freespace finder searches for an area where RAM mirrors aren't available (banks $40 to $6F and $F0 to $FF). If no such area is found, it searches in the remaining banks ($10 to $3F). Recommended when inserting data. |
| `align` | The freespace finder searches for an area at the beginning of a bank. |
| `cleaned` | Suppresses the warning about freespace leaking. Useful when Asar's leak detection misbehaves on an autoclean with a complicated math statement or similar. |
| `static` | Prevents the freespace area from moving once assigned. This also prevents it from growing (an error is thrown if the area would need to grow). Useful in situations where data needs to remain in a certain location (for example: when another tool or another patch needs to access it). |
| `value` | A number literal or math statement specifying the byte value to look for when searching for freespace (default: $00). To find freespace, Asar will look for continuous areas of this value. When using autoclean on this freespace, this is also the value the area will be cleaned to. Note that specifying the byte like this is deprecated. You should use the separate `freespacebyte` command instead. |

TODO: document `segment`
  
The `freecode` command is an alias of `freespace ram`, whreas the `freedata` command is an alias of `freespace noram`. One thing to note about freespaces is that if Asar places two freespace areas within the same bank, it will use 24-bit addressing in cases where they reference each other, despite 16-bit addressing being possible in theory. This can be worked around by only using a single freespace area instead. It's not recommended to explicitly use 16-bit addressing in these cases as the two freespace areas are not guaranteed to always end up in the same bank for all users.

```asar
; Let's assume this to be some location in the ROM originally containing
;lda #$10
;sta $1F
org $01A56B
    autoclean jsl MyNewCode
    
freecode

MyNewCode:
    ; Do something here
    ; ...
    
.Return:
    ; We overwrote some code from the original ROM with our org, so we have to restore it here
    lda #$10
    sta $1F
    
    rtl
```

## `freespacebyte`

{{# syn: freespacebyte {value} #}}

This command sets the byte which Asar considers to be free space. This value will be used for searching for freespace, as padding when resizing the ROM, or when cleaning up old freespaces.

## `autoclean`

{{# syn:
autoclean jml/jsl/lda/cmp/.../dl {label}
autoclean {snes_address}
#}}

The autoclean command makes it possible for Asar to automatically clean up and reuse all of the freespace allocated by a patch when applying that patch again. The purpose of this is to prevent freespace leaks. Normally, applying a patch including a freespace (or similar) command to the same ROM multiple times would allocate a new freespace area each time. Since Asar automatically protects allocated freespace via RATS tags, all freespace areas previously allocated by the same patch would leak and become unusable, making the output ROM run out of freespace eventually. The autoclean command can prevent this by freeing up freespace areas previously allocated by the patch before allocating new ones. How it accomplishes this depends on how it is used:

-   **When used with an assembly instruction (most commonly `jml` or `jsl`):**  
    The `label` parameter must be a label pointing to inside a freespace area. The instruction can be any instruction that supports long (24-bit) addressing. When the patch is applied and the autoclean is encountered, Asar checks whether the output ROM contains the same instruction at the current pc. If it does, and the instruction points into a freespace area with a valid RATS tag, the RATS tag is removed along with its contents.
-   **When used with a `dl`:**  
    The `label` parameter must be a label pointing to inside a freespace area. When the patch is applied and the autoclean is encountered, Asar checks whether the output ROM contains an address pointing to the expanded area of the ROM (banks $10+) at the current pc. If it does, Asar checks whether that address points to an area protected by a RATS tag (including the RATS tag itself). If it does, Asar cleans up that area and removes the RATS tag.
-   **When used with just an address:**  
    The `snes_address` parameter must be any label, number literal or math statement evaluating to an SNES address pointing to inside a freespace area. When the patch is applied and the autoclean is encountered, Asar checks whether that address points to the expanded area of the ROM (banks $10+). If it does, Asar checks whether it points to an area protected by a RATS tag (including the RATS tag itself). If it does, Asar cleans up that area and removes the RATS tag.

When using autoclean with an instruction or dl, Asar will also assemble the respective line of code at the current pc. For simplicity, you can treat the autoclean command like a modifier in those cases. A few more things to note when using the autoclean command:

-   The autoclean command itself may not be used inside a freespace area. To automatically clean up freespace that is only referenced within another freespace area, you can use the [`prot`](#prot) command.
-   It is safe to have multiple autoclean commands pointing to the same freespace area.
-   You can not use autoclean with a label pointing to the very end of a freespace area.

```asar
; Let's assume this to be some location in the ROM containing a function pointer table or similar
org $00A5F2
    autoclean dl MyNewFunction1
    autoclean dl MyNewFunction2
    
freecode

MyNewFunction1:
    ; ...
    rtl
    
MyNewFunction2:
    ; ...
    rtl
```

## `prot`

{{# syn: prot {label}[,label...] #}}

The prot command makes it possible for Asar to automatically clean up a freespace area that is only referenced within another freespace area and thus can't be cleaned via an autoclean directly. It must be used at the beginning of a freespace area (right after the freespace command), where the `label` parameter must be a label pointing to inside a freespace area (you can pass up to 85 labels separated by commas to a single prot). When a freespace area containing a prot is cleaned by an autoclean, all freespace areas referenced by the prot are also cleaned up.

```asar
org $0194BC
    autoclean jsl MyNewFunction
    
    
freecode
prot SomeLargeData

MyNewFunction:
    ldx.b #0
    
.Loop:
    {
        lda SomeLargeData,x
        cmp #$FF
        beq .Return
        
        ; ...
        
        inx
        
        bra .Loop
    }
    
.Return:
    rtl
    
    
freedata

SomeLargeData:
    db $00,$01,$02,$03
    ; ...
    db $FF
```
