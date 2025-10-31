l = $006000
l2 = $016000

hirom
org $008000
optimize address mirrors
;`08000
;`ad 00 60
lda l
;`af 00 60 01
lda l2
