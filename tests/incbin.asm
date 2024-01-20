;`56 78
;`12 34
;`05 06 07
org $8000
incbin "data/64kb.bin":$8000..$8002
incbin "data/64kb.bin":$7FFE..$8000
incbin "data/filename with spaces.bin":2+2..8-1

