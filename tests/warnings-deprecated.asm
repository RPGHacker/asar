;`warnWinvalid_warning_id
;`01
; what is the point of this test????

org $008000
warnings push
warnings disable Einvalid_warning_id

db $01

warnings pull
