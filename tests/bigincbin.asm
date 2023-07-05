;`+
;`07606 5C 15 80 90
;`07FD7 0A
;`
;`80000
;`53 54 41 52 10 00 EF FF
;`50 52 4F 54 03 00 80 91
;`53 54 4F 50 00
;`AF 00 80 91
;`
;`87FF8
;`53 54 41 52 FF FF 00 00
;`88000 89 AB
;`8FFFE 12 34
;`90000 56 78
;`97FFE CD EF
;`FFFFF 00
;`warnWfeature_deprecated

org $00F606
autoclean JML Mymain
freecode
prot Derp
Mymain:
incbin "data/64kb.bin" -> Derp
LDA Derp
