;`+ #2
;`000000
;`22 09 80 90
;`22 13 80 90
;`000020
;`22 09 80 90
;`07FD7 0A
;`080000 53 54 41 52 01 00 FE FF 01 01 53 54 41 52 01 00
;`080010 FE FF 02 02
;`FFFFF 00

;print "previous test1: $",hex(read3($008000+1))
;print "previous test2: $",hex(read3($008004+1))
;print "new test1: $",hex(test1)
;print "new test2: $",hex(test2)

org $008000
autoclean JSL test1
autoclean JSL test2

freecode
db 1
test1: db 1

org $008020
autoclean JSL test1

freecode
db 2
test2: db 2
