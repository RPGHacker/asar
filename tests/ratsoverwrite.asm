;`+ #2
;`80000
;`53 54 41 52 02 00 fd ff 01 02 03
;`42 00 00 00
;`53 54 41 52 02 00 fd ff 04 05 06
;`07 08 09 0a 0b 0c
;`01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01
;`fffff 00

; make sure the freespace allocator doesn't overwrite stuff it's not supposed to

if not(canread1($108000))
    ; first pass
    freecode cleaned
    db 1,2,3

    segment
    fill 4

    freecode cleaned
    db 4,5,6
else
    ; second pass
    segment
    db 7,8,9,10,11,12

    segment
    fillbyte 1 : fill 24

    segment
    db $42
endif
