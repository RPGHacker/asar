;`34 12 7E
;`00 80 00
;`34 12 7E

org $008000
base $7E1234
label:
label_pc = realbase()
label3 = pc()

dl label
dl label_pc
dl label3
