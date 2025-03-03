# Mapping Modes

Asar supports a number of different mapping modes. They control the address translation used by Asar during compilation (aka where in the output file Asar writes to). Historically, SNES cartridges used a number of different mappers to address data in ROM. Those mappers can be supported by using the respective mapping mode in Asar. It's possible, but not recommended, to use different mapping modes on the same ROM. Detailed explanations on each mapping mode are beyond the scope of this manual, so please check the SNES Dev Manual or other specialized resources for that.

NOTE: Changing the mapper after having previously set it will generate warning `Wmapper_already_set`.  

<!-- TODO probably a good idea to document the sa1 mapping modes better -->
  
- {{#cmd: lorom #}}: Switch to LoROM mapping mode.
- {{#cmd: hirom #}}: Switch to HiROM mapping mode.
- {{#cmd: exlorom #}}: Switch to ExLoROM mapping mode.
- {{#cmd: exhirom #}}: Switch to ExHiROM mapping mode.
- {{#cmd: sa1rom [num, num, num, num] #}}: Switch to hybrid SA-1 mapping mode. To tell which banks are mapped in (maximum is 7) use the optional parameter, like so: `sa1rom 0,1,4,6`. The default is 0,1,2,3.
- {{#cmd: fullsa1rom #}}: Switch to full SA-1 mapping mode.
- {{#cmd: sfxrom #}}: Switch to Super FX mapping mode.
- {{#cmd: norom #}}: Disable Asar's address translation; the SNES address is equal to the PC address. Can be combined with `base` and macros to implement your own address translation.

 When no mapping mode is specified, Asar tries to determine the mapping mode from the output ROM. If that isn't possible, Asar defaults to `lorom`.

 ```asar
 lorom
org $008000
db $FF      ; Will write to PC address 0x000000

hirom
org $008000
db $FF      ; Will write to PC address 0x008000
```
