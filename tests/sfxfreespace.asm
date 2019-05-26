;`#2
;`80000 53 54 41 52 03 00 FC FF
;`EA EA EA EA
;`1FFFFF 00

sfxrom
; slight hack to do different stuff on 2nd invocation (sfxrom doesn't have autoexpanding)
if canread1($8000)
    freecode cleaned
    NOP #4
else
    org $3fffff
    db $00
endif
