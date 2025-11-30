;`errEinvalid_opcode_length
;`errEmacro_label_outside_of_macro
;`errEmacro_label_outside_of_macro
;`errEdivision_by_zero
org $008000
lda.d #$10
?asdf:
bne ?-

org $008000
dl 1/(x-$008003)
x:
