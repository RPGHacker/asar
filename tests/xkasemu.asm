;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`53 54 41 52
;`21 61
;`BF 0A 80 00
;`
;`01 02 03 03

;@xkas
org $008000

db "STAR"

db "!a"

LDA derp,x
derp:

incbin .\include\dummy.asm

rep -1 : db 0
rep 0 : db 1
rep 1 : db 2
rep 2 : db 3
