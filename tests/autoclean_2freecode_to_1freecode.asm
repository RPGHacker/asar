;`+ #2
;`000000
;`22 08 80 90
;`22 09 80 90
;`07FD7 0A
;`80000
;`53 54 41 52 01 00 FE FF 6B 6B
;`FFFFF 00


org $008000

if read1($008000) != $22

; print "first run"

autoclean jsl hijack1
autoclean jsl hijack2

freecode
hijack1:
rtl

freecode
hijack2:
rtl

else

; print "second run"

autoclean jsl hijack1
autoclean jsl hijack2

freecode
hijack1:
rtl
hijack2:
rtl

endif
