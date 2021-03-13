;`34 12 7E
;`00 80 00

org $008000
base $7E1234
label:
label_pc = realbase()

dl label
dl label_pc
