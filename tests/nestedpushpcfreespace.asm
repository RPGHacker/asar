;`+
;`07FD7 0A
;`80000
;`53 54 41 52 05 00 FA FF
;`01 02 03 04 05 06
;`53 54 41 52 02 00 FD FF
;`07 08 09
;`53 54 41 52 02 00 FD FF
;`0A 0B 0C
;`FFFFF 00

freedata cleaned
db 1,2,3

pushpc
freedata cleaned
db 7,8,9
pullpc

db 4,5,6
Label:

freedata cleaned
db 10,11,12