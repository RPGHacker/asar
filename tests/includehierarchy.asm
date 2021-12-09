;`01 02 03 04 05 06 07 08 09
;`04 05 06

asar 1.60

org $008000

db $01
incsrc "include/data.asm"
db $09

%include_in_macro()
