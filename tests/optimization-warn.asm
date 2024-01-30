;`ad 12 00 af 13 00 01 af 56 34 12 20 12 00
;`warnWoptimization_settings
;`warnWoptimization_settings
;`warnWoptimization_settings
org $8000
lbl = $12
lbl2 = $010013
lbl3 = $123456
lda lbl
lda lbl2
lda lbl3
; this does not give the warning
jsr lbl
