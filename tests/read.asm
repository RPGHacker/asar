;`+
;`00000 01 79 7A 9C 9F 00 42 46 9C 0C 42
;`80000
;must be in "patch-to-smw" mode since Asar considers it to read outside the ROM otherwise

check title "SUPER MARIOWORLD     "

org $008000
db $01
db read1($008000)+1
dw read2($008000)+2
dl read3($008001)+3
dd read4($008003)+4
