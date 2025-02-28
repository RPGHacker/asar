org $008000
main:
; bank extract
;`A9 03 A9 03 A9 03 A9 03 A9 03 A9 03
lda #<:test_label
lda #bank(test_label)
lda #test_label >> 16
lda #test_label / 65536
lda #test_label / $10000
lda #test_label >> $10
;`A9 00 80
; this one isn't a bankextr
lda #main | 0 >> 16

;`A9 00 A9 00 00
lda #%00000000
lda #%0000000000000000

;`A9 FF A9 00 01 A9 01 01
lda #255
lda #256
lda #257

;`A9 01 A9 12 A9 23 01 A9 34 12
lda #$1
lda #$12
lda #$123
lda #$1234

;`B5 00 B5 00
sin = $008000
cos = $018000
lda sin(1),x
lda cos(1),x

test_label = $038000

function wat() = test_label + 5
;`AF 05 80 03
lda wat()

; would be neat if this worked...
;function bankwhat(x) = bank(x + $1234)
;;`A9 03
;lda #bankwhat(test_label)

base $018000
;`AD 00 80
lda pc()
;`AF 31 80 00
lda realbase()
